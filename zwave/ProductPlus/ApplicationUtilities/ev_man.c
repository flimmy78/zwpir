/**
 *
 * Copyright (c) 2001-2014
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 * @file ev_man.h.c
 *
 * @brief Framework event manager, handling all event types for the framework
 *
 * @author: Thomas Roll
 *
 * Last Changed By: $Author: tro $
 * Revision: $Revision: 31775 $
 * Last Changed: $Date: 2015-08-14 12:44:12 +0200 (Fri, 14 Aug 2015) $
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ev_man.h>
#include <ZW_task.h>
#include <ZW_util_queue_api.h>
#include <misc.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
/**
 * @def ZW_DEBUG_EV_MAN_SEND_BYTE(data)
 * Transmits a given byte to the debug port.
 * @def ZW_DEBUG_EV_MAN_SEND_STR(STR)
 * Transmits a given string to the debug port.
 * @def ZW_DEBUG_EV_MAN_SEND_NUM(data)
 * Transmits a given number to the debug port.
 * @def ZW_DEBUG_EV_MAN_SEND_WORD_NUM(data)
 * Transmits a given WORD number to the debug port.
 * @def ZW_DEBUG_EV_MAN_SEND_NL()
 * Transmits a newline to the debug port.
 */
#ifdef ZW_DEBUG_EV_MAN
#include <ZW_uart_api.h>
#define ZW_DEBUG_EV_MAN_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_EV_MAN_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_EV_MAN_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_EV_MAN_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_EV_MAN_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_EV_MAN_SEND_BYTE(data)
#define ZW_DEBUG_EV_MAN_SEND_STR(STR)
#define ZW_DEBUG_EV_MAN_SEND_NUM(data)
#define ZW_DEBUG_EV_MAN_SEND_WORD_NUM(data)
#define ZW_DEBUG_EV_MAN_SEND_NL()
#endif

#define JOB_QUEUE_BUFFER_SIZE  3


/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/**
 * pointer to application state event machine
 */
VOID_CALLBACKFUNC(m_pApplicationStateMachine)(BYTE) = NULL;

/**
 * Event queue
 */
static QUEUE_T eventQueueEngine;

/**
 * Event queue
 */
static QUEUE_T jobQueueEngine;

/**
 * Event queue buffer
 */
BYTE eventQueueBuffer[EVENT_QUEUE_BUFFER_SIZE];


/**
 * Event queue buffer
 */
BYTE jobQueueBuffer[JOB_QUEUE_BUFFER_SIZE];

/**
 * Task handler ID
 */
static BYTE taskHandleId = 0;


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/




/**
 * @brief Initializes event scheduler.
 */
void
EventSchedulerInit(VOID_CALLBACKFUNC(pApplicationStateMachine)(BYTE))
{
  if(NON_NULL( pApplicationStateMachine ))
  {
    m_pApplicationStateMachine = pApplicationStateMachine;
    ZW_util_queue_Init(&eventQueueEngine, eventQueueBuffer, sizeof(eventQueueBuffer[0]), EVENT_QUEUE_BUFFER_SIZE);
    ZW_util_queue_Init(&jobQueueEngine, jobQueueBuffer, sizeof(jobQueueBuffer[0]), JOB_QUEUE_BUFFER_SIZE);

    if((taskHandleId = TaskAdd(ZCB_EventScheduler, (const char*)"AppEvScheduler")) == 0)
    {
      ZW_DEBUG_EV_MAN_SEND_NL();
      ZW_DEBUG_EV_MAN_SEND_STR("Task pool full");
    }
  }
}

PCB_BOOL(ZCB_EventSchedulerEventAdd)(BYTE event)
{
  BOOL status = FALSE;
  if(NON_NULL( m_pApplicationStateMachine ))
  {
    if((status = ZW_util_queue_Enqueue(&eventQueueEngine, &event))== FALSE)
    {
      ZW_DEBUG_EV_MAN_SEND_NL();
      ZW_DEBUG_EV_MAN_SEND_STR("Queue full!!!!!!!!");
      ZW_DEBUG_EV_MAN_SEND_NL();
    }
  }
  return status;
}


/**
 * @brief Processes events.
 */
PCB_BOOL(ZCB_EventScheduler)(void)
{
  if(NON_NULL( m_pApplicationStateMachine ))
  {
    BYTE event;
    if(TRUE == ZW_util_queue_Dequeue(&eventQueueEngine, &event))
    {
      m_pApplicationStateMachine(event);
    }
  }
  return TRUE;
}

PCB_BOOL(ZCB_EventEnqueue)(BYTE event)
{
  ZW_DEBUG_EV_MAN_SEND_NL();
  ZW_DEBUG_EV_MAN_SEND_STR("ZCB_EventEnqueue");
  ZW_DEBUG_EV_MAN_SEND_NUM(event);
  ZW_DEBUG_EV_MAN_SEND_NL();
  return ZW_util_queue_Enqueue(&jobQueueEngine, &event);
}

PCB_BOOL(ZCB_EventDequeue)(BYTE* pEvent)
{
  BOOL x = ZW_util_queue_Dequeue(&jobQueueEngine, pEvent);
  ZW_DEBUG_EV_MAN_SEND_NL();
  ZW_DEBUG_EV_MAN_SEND_STR("ZCB_EventDequeue");
  ZW_DEBUG_EV_MAN_SEND_NUM(*pEvent);
  ZW_DEBUG_EV_MAN_SEND_NUM(x);
  ZW_DEBUG_EV_MAN_SEND_NL();
  return x;
}
