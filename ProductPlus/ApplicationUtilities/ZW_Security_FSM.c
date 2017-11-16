/*************************************************************************** 
* 
* Copyright (c) 2013
* Sigma Designs, Inc. 
* All Rights Reserved 
* 
*--------------------------------------------------------------------------- 
* 
* Description:
*
*   Finite state machine (FSM) for the security engine. The primary purpose
*   is to "smooth" the frame flow and avoid deadlocks in long communication
*   sessions (e.g. firmware updates).
*
*   This is achieved in two ways:
*     a) Holding back outgoing Nonce Reports while another security session
*        is in progress
*     b) Delaying delivery of incoming secure messages to the application while 
*        another security session is in progress. This prevents the application
*        from trying to send a secure response while the security module is
*        busy.
*        An improvement would be to deliver the incoming message, accept the
*        outgoing response and caching it until the security module is ready
*        to send it (see TODO below).
*
*   FSM Template taken from
*   http://stackoverflow.com/questions/1647631/c-state-machine-design
*
*   This module interacts with the modules ZW_Security_AES_module and 
*   ZW_TransportSecurity. These modules check the FSM state via calls to
*   GetSecurityFsmState(). And they change the FSM state via PostEvent().
*
* 
* Author:   Jakob Buron 
* 
* Last Changed By:  $Author: tro $ 
* Revision:         $Revision: 33251 $ 
* Last Changed:     $Date: 2016-03-14 09:35:49 +0100 (ma, 14 mar 2016) $ 
* 
****************************************************************************/
#include <ZW_Security_FSM.h>
#include <ZW_uart_api.h>
#include <ZW_Security_AES_module.h>
#include <ZW_timer_api.h>

#ifdef ZW_DEBUG_SECURITY
#define ZW_DEBUG_SECURITY_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_SECURITY_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_SECURITY_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_SECURITY_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_SECURITY_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_SECURITY_SEND_BYTE(data)
#define ZW_DEBUG_SECURITY_SEND_STR(STR)
#define ZW_DEBUG_SECURITY_SEND_NUM(data)
#define ZW_DEBUG_SECURITY_SEND_WORD_NUM(data)
#define ZW_DEBUG_SECURITY_SEND_NL()
#endif

/* TODO: Cache and delay the outgoing transmission instead of rejecting it.
*        What if you are simultaneously holding back an outgoing Nonce Report
*        and an outgoing transmission? Which should be serviced first? */

#define NO_TIMER 0xFF /* Unused timer handle */
#define NO_TIMEOUT 0 /* No timeout from this state */

#define STATE enum SECURITY_FSM_STATES

/* This define forces the function into the inter-bank call table.
 * Is is a prerequisite for all callback functions. */
#define ZCB(func) void func(void); \
  code const void (code * func ## _p)(void) = &func;

typedef struct {
    STATE st;             /* Current state */
    SECURITY_FSM_EV_T ev;             /* Incoming event */
    STATE next_st;
    void (CODE *fn)(void);    /* Action function returning next state */
} transition_t;

typedef struct {
  STATE st;
  WORD timeout;        /* timeout in 10 ms ticks */
  VOID_CALLBACKFUNC(tfn)(void);   /* timeout callback function */
} state_timeout_t;

static STATE state = ST_IDLE;
static BYTE bFsmTimerHandle = NO_TIMER;
static BYTE bFsmTimerCountdown;
static VOID_CALLBACKFUNC(cbFsmTimer)(void);
static BYTE i; /* temp variable */

/* Function prototypes needed in transition and timeout tables */
void ReturnToIdle(void);
void FsmWarning(void);
void FsmError(void);
void ZCB_TimerReturnToIdle(void);

/* State transition table
 * This is where the state machine is defined.
 *
 * Each transition can optionally trigger a call to an action function.
 * An action function defines the actions that must occur when a particular
 * transition occurs.
 *
 * Format: {Current state, incoming event, next state, action_function} */
static const transition_t trans[] = {
    { ST_SENDING_NONCE_GET, EV_NONCE_REPORT_ARRIVES, ST_SENDING_ENCAP, NULL },  /* Nonce Report can arrive before ACK on Nonce Get */
    { ST_AWAITING_ENCAP, EV_SEC_ENCAP_ARRIVES, ST_IDLE, &ReturnToIdle },
    { ST_AWAITING_ENCAP, EV_RETURN_TO_IDLE, ST_IDLE, &ReturnToIdle},
    { ST_IDLE, EV_NONCE_GET_TRANSMITTING, ST_SENDING_NONCE_GET, NULL },
    { ST_AWAITING_NONCE_REPORT, EV_NONCE_REPORT_ARRIVES, ST_SENDING_ENCAP, NULL },
    { ST_AWAITING_NONCE_REPORT, EV_RETURN_TO_IDLE, ST_IDLE, &ReturnToIdle },
    { ST_SENDING_ENCAP, EV_SEC_ENCAP_TRANSMITTED, ST_IDLE, &ReturnToIdle },
    { ST_SENDING_ENCAP, EV_RETURN_TO_IDLE, ST_IDLE, &ReturnToIdle },
    { ST_SENDING_NONCE_GET, EV_RETURN_TO_IDLE, ST_IDLE, &ReturnToIdle },
    { ST_SENDING_NONCE_GET, EV_NONCE_GET_TRANSMIT_OK, ST_AWAITING_NONCE_REPORT, NULL },
    { ST_IDLE, EV_NONCE_REPORT_TRANSMITTING, ST_SENDING_NONCE_REPORT, NULL },
    { ST_SENDING_NONCE_REPORT, EV_NONCE_REPORT_TRANSMIT_OK, ST_AWAITING_ENCAP, NULL },
    { ST_IDLE, EV_NONCE_REPORT_TRANSMIT_OK, ST_AWAITING_ENCAP, &FsmWarning }, /* Can this even happen? */
    { ST_IDLE, EV_SEC_ENCAP_ARRIVES, ST_IDLE, NULL },
    { ST_ANY, EV_NONCE_REPORT_ARRIVES, ST_UNCHANGED, NULL }, /* This event should be ignored in all states
                                                                except those previously mentioned */
    { ST_ANY, EV_ANY, ST_UNCHANGED, &FsmError }
};
#define TRANS_COUNT (sizeof(trans)/sizeof(*trans))

/* States can have a default timeout. If the FSM remains in the same
 * state for the specified duration, the specified callback function
 * will be called.
 * Format: {STATE, timeout in milliseconds, callback_function}
 * Only states listed here have timeouts */
static state_timeout_t timeoutTable[] = {
    {ST_AWAITING_NONCE_REPORT, 2000, ZCB_TimerReturnToIdle},
    {ST_SENDING_ENCAP, 2000, ZCB_TimerReturnToIdle},
    {ST_AWAITING_ENCAP, 20, ZCB_TimerReturnToIdle},
    {ST_SENDING_NONCE_GET, 2000, ZCB_TimerReturnToIdle},
    {ST_SENDING_NONCE_REPORT, 2000, ZCB_TimerReturnToIdle},
};
#define TIMEOUT_TABLE_COUNT (sizeof(timeoutTable)/sizeof(*timeoutTable))

/*============================   FsmError   ===============================
**
**  Action function called when an unexpected event is received.
**
**  Side effects: Re-initializes the FSM.
**
**-------------------------------------------------------------------------*/
ZCB(FsmError)
void FsmError()
{
  ZW_DEBUG_SECURITY_SEND_STR("FsmError: Unexpected event in this state\r\n");
  InitSecurityFsm();
}

/*==========================   ReturnToIdle   =============================
**
**  Action function called by all transitions that should return the FSM
**  to idle state.
**
**  Side effects: FSM is returned to idle. Security module is notified.
**
**-------------------------------------------------------------------------*/
ZCB(ReturnToIdle)
void ReturnToIdle(void)
{
  /* Notify security module that we are returning to idle */
  SecurityReturnToIdle();
}

/*=======================   ZCB_TimerReturnToIdle   ========================
**
**  Timeout function that returns the FSB to idle.
**  Called by the FSM timer.
**
**  Side effects: None
**
**-------------------------------------------------------------------------*/
ZCB(ZCB_TimerReturnToIdle)
void ZCB_TimerReturnToIdle()
{
  PostEvent(EV_RETURN_TO_IDLE);
}

/*============================   FsmWarning   ===============================
**
**  Action function used by transitions that wish to print a warning
**  to the debut output.
**
**  Side effects: None
**
**-------------------------------------------------------------------------*/
ZCB(FsmWarning)
void FsmWarning()
{
  ZW_DEBUG_SECURITY_SEND_STR("FsmWarning: Unexpected event in this state\r\n");
}

/*============================   StopTimer   ===============================
**
**  Cancels the FSM timer.
**
**  Side effects: None
**
**-------------------------------------------------------------------------*/
static void StopTimer()
{
  if (bFsmTimerHandle != NO_TIMER)
  {
    ZW_TIMER_CANCEL(bFsmTimerHandle);
    bFsmTimerHandle = NO_TIMER;
  }
}

/*===========================   ZCB_FsmTimer   ==============================
**
**  Timer callback function for the default FSM timer.
**  If a timeout and a callback function is specified in the
**  timeoutTable, ths callback function will be called automatically
**  by this timer function.
**
**  The callback function will only be called if no state change has occurred
**  within the specified timeout since entering the current state.
**
**  Side effects: None
**
**-------------------------------------------------------------------------*/
ZCB(ZCB_FsmTimer)
void ZCB_FsmTimer()
{
  ZW_DEBUG_SECURITY_SEND_BYTE(' ');
  ZW_DEBUG_SECURITY_SEND_STR("FsmTimer");
  ZW_DEBUG_SECURITY_SEND_NUM(bFsmTimerCountdown);
  ZW_DEBUG_SECURITY_SEND_BYTE(' ');

  if (bFsmTimerCountdown > 0)
  {
    bFsmTimerCountdown--;
    return;
  }
  /* The timer repeat count should already have stopped the timer,
   * but better safe than sorry. */
  StopTimer();
  if (NON_NULL(cbFsmTimer))
  {
    cbFsmTimer();
  }
}

/*============================   StartTimer   ===============================
**
**  Start the FSM timer. Note the higher resolution if timeout is
**  shorter than or equal to 2.55 seconds.
**
**  Side effects: Cancels running FSM timer if any.
**
**-------------------------------------------------------------------------*/
static void StartTimer(
    WORD timeout,                  /* IN Unit: 10 ms ticks. If over 255, resolution is 1 second */
    VOID_CALLBACKFUNC(cbFunc)(void))     /* IN timeout callback function */
{
  StopTimer();
  if (timeout > 255)
  {
    /* Timeout larger than single timeout. */
    /* Convert timeout from 10 ms ticks to seconds*/
    bFsmTimerCountdown = timeout / 100;
    cbFsmTimer = cbFunc;
    bFsmTimerHandle = ZW_TIMER_START(ZCB_FsmTimer, TIMER_ONE_SECOND, bFsmTimerCountdown);
  }
  else
  {
    /* timeout is in range 10..2550 ms */
    bFsmTimerHandle = ZW_TIMER_START(cbFunc, timeout, TIMER_ONE_TIME);
  }
  if (bFsmTimerHandle == NO_TIMER)
  {
    ZW_DEBUG_SECURITY_SEND_STR("Failed to get timer for FsmTimer +++++\r\n");
  }
  else
  {
    ZW_DEBUG_SECURITY_SEND_STR("FSM got handle ");
    ZW_DEBUG_SECURITY_SEND_NUM(bFsmTimerHandle);
    ZW_DEBUG_SECURITY_SEND_NL();
  }
}

/*=========================   StartStateTimer   ============================
**
**  Find and start the state timer for current state, if one has been
**  defined in timeoutTable.
**
**  Side effects: Cancels the currently running state timer.
**
**-------------------------------------------------------------------------*/
static void StartStateTimer()
{
  for (i = 0; i < TIMEOUT_TABLE_COUNT; i++)
  {
    if (state == timeoutTable[i].st)
    {
      StartTimer(timeoutTable[i].timeout, timeoutTable[i].tfn);
    }
  }
}

/*=========================   InitSecurityFsm   ============================
**
**  Initialized the finite state machine.
**
**  Side effects: None
**
**-------------------------------------------------------------------------*/
void InitSecurityFsm()
{
  StopTimer();
  state = ST_IDLE;
}

/*============================   PostEvent   ===============================
**
**  Post an event to the finite state machine. Update the state,
**  run action function associated with the transition and start
**  the state timer for the next state.
**
**  Side effects: None
**
**-------------------------------------------------------------------------*/
void PostEvent(SECURITY_FSM_EV_T event)
{
  BYTE i;
  SECURITY_FSM_STATE_T old_state;
  for (i = 0; i < TRANS_COUNT; i++)
  {
    if ((state == trans[i].st) || (ST_ANY == trans[i].st))
    {
      if ((event == trans[i].ev) || (EV_ANY == trans[i].ev))
      {
        old_state = state;
        if (ST_UNCHANGED != trans[i].next_st)
        {
          state = trans[i].next_st;
        }
        ZW_DEBUG_SECURITY_SEND_STR("PostEvent: ev ");
        ZW_DEBUG_SECURITY_SEND_NUM(event);
        ZW_DEBUG_SECURITY_SEND_STR(", st ");
        ZW_DEBUG_SECURITY_SEND_NUM(old_state);
        ZW_DEBUG_SECURITY_SEND_STR(" -> ");
        ZW_DEBUG_SECURITY_SEND_NUM(state);
        ZW_DEBUG_SECURITY_SEND_STR(" idx ");
        ZW_DEBUG_SECURITY_SEND_NUM(i);
        ZW_DEBUG_SECURITY_SEND_NL();
        /* Only start timer when we move to a NEW state*/
        if (old_state != state)
        {
          StopTimer();
          StartStateTimer();
        }
        if(NON_NULL(trans[i].fn))
        {
          (trans[i].fn)();
        }
        break;
      }
    }
  }
}

/*=======================   GetSecurityFsmState   ==========================
**
**  Return the current state of the state machine.
**
**  Side effects: None
**
**-------------------------------------------------------------------------*/
SECURITY_FSM_STATE_T GetSecurityFsmState()
{
  return state;
}
