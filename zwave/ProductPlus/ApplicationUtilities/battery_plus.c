/**
 * @file battery_plus.c
 * @brief Handling of power functionality and helper module for the Wake Up CC.
 * @author Thomas Roll
 * @author Christian Salmony Olsen
 * @copyright Copyright (c) 2001-2016
 * Sigma Designs, Inc.
 * All Rights Reserved
 * @details Implements functions that make is easier to support Battery Operated Nodes.
 */

#define ZW_BEAM_RX_WAKEUP

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_typedefs.h>
#include <ZW_stdint.h>
#include <ZW_power_api.h>
#include <eeprom.h>
#include <battery_plus.h>
#include <ZW_uart_api.h>
#include <CommandClassPowerLevel.h>
#include "misc.h"

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_BATT
#define ZW_DEBUG_BATT_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_BATT_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_BATT_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_BATT_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_BATT_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_BATT_SEND_BYTE(data)
#define ZW_DEBUG_BATT_SEND_STR(STR)
#define ZW_DEBUG_BATT_SEND_NUM(data)
#define ZW_DEBUG_BATT_SEND_WORD_NUM(data)
#define ZW_DEBUG_BATT_SEND_NL()
#endif

#ifndef TRANSMIT_OPTION_EXPLORE       /* if library do not support explorer frame, ignore it: */
#define TRANSMIT_OPTION_EXPLORE   0
#endif

#ifndef SEC_2_POWERDOWNTIMEOUT
#define SEC_2_POWERDOWNTIMEOUT 20
#endif

#ifndef MSEC_200_POWERDOWNTIMEOUT
#define MSEC_200_POWERDOWNTIMEOUT 2
#endif

#ifndef SEC_10_POWERDOWNTIMEOUT
#define SEC_10_POWERDOWNTIMEOUT 100
#endif

#define MAX_WUT_TIME 0xff

/* 100 milsecond timeout */
#define TIMER_100_MSEC   10

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/* WUT count, when decreased to 0, wake up */
extern XDWORD wakeupCount;

//WAKEUP_DATA intervalData;
BYTE powerDownTicks;

/* Keep alive time between power downs */
BYTE powerDownTimerHandle = 0xFF;

XDWORD sleepPeriod = 0; /* Wakeup timeout - number of seconds before wakeup */
XDWORD sleepStepTime = 0;
BYTE sleepStepRestTime = 0;
BATT_MODE batteryMode = BATT_MODE_NOT_LISTENING;

/**
 * ActiveWakeUpState indicate if tells id Command Class WakeUp is active. if
 * active, power down time is 10 seconds!
 */
BOOL ActiveWakeUpState = FALSE;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

void ZCB_PowerDownTimeoutFunction(void);

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

static void CalcStepTimeRestTime();
void PowerDownNow(void);
void ZCB_ResetPowerDownTimeout(BYTE timeout);
void ZCB_ClearSetPowerDownTimeout(BYTE timeout);

uint32_t
handleWakeUpIntervalGet(void)
{
  return sleepPeriod;
}

void
handleWakeupNoMoreInfo(void)
{
  ZW_DEBUG_BATT_SEND_STR("handleWakeupNoMoreInfo");
  ZW_DEBUG_BATT_SEND_NUM(powerDownTicks);
  ZW_DEBUG_BATT_SEND_NL();
  /* Tell application WAKE up state is finish." */
   powerDownTicks = 0; /*secure routed ack is received*/
   /*Start the timer if it is not already started*/
   ZCB_StartPowerDownTimer();
   ZCB_PowerDownTimeoutFunction();
}

/**
 * @brief Checks whether it's time to power down. If it is, the framework and application are asked
 * whether they're ready for power down. If they are, the function powers down the chip.
 */
PCB(ZCB_PowerDownTimeoutFunction)(void)
{
  ZW_DEBUG_BATT_SEND_BYTE('P');
  ZW_DEBUG_BATT_SEND_BYTE('d');
  ZW_DEBUG_BATT_SEND_NUM(powerDownTicks);
  ZW_DEBUG_BATT_SEND_NL();

  if (!powerDownTicks)
  {
    /*
     * Checks whether the following firmware parts are ready to power down:
     * - The application
     * - The transport layer (mutex)
     * - Powerlevel command class
     */
    if ((TRUE == AppPowerDownReady()) &&
        (FALSE == ActiveJobs()) &&
        (FALSE == CommandClassPowerLevelIsInProgress()))
    {
#ifdef ZW_DEBUG
      // If we're using debug output, remember to flush to UART.
      while(ZW_DEBUG_TX_STATUS());
#endif

      PowerDownNow();
    }
  }
  else
  {
    powerDownTicks--;
  }
}

PCB(ZCB_StartPowerDownTimer)(void)
{
  if (powerDownTimerHandle == 0xFF)
  {
    powerDownTimerHandle = ZW_TIMER_START(ZCB_PowerDownTimeoutFunction,
                           TIMER_100_MSEC,
                           TIMER_FOREVER);
    ZW_DEBUG_BATT_SEND_NUM(powerDownTimerHandle);
  }
}

PCB(ZCB_StopPowerDownTimer)(void)
{
  ZW_DEBUG_BATT_SEND_BYTE('-');
  if (powerDownTimerHandle != 0xFF)
  {
    ZW_DEBUG_BATT_SEND_BYTE('-');
    ZW_TIMER_CANCEL(powerDownTimerHandle);
    powerDownTimerHandle = 0xFF;
  }
}

PCB(ZCB_SetPowerDownTimeout)(BYTE timeout)
{
  if (powerDownTicks < timeout)
  {
    ZW_DEBUG_BATT_SEND_NL();
    ZW_DEBUG_BATT_SEND_STR("New timeout val: ");
    ZW_DEBUG_BATT_SEND_NUM(timeout);
    powerDownTicks = timeout;
  }
}

PCB(ZCB_ResetPowerDownTimeout)(BYTE timeout)
{
  ZCB_SetPowerDownTimeout(timeout);
  ZCB_StartPowerDownTimer();
}

/**
 * @brief Sets the power down timeout.
 * @details The value specifies the time the device will stay wake before it goes to power down
 * state. Setting the value will increase/decrease the current timeout value.
 */
PCB(ZCB_ClearSetPowerDownTimeout)(BYTE timeout)
{
  ZW_DEBUG_BATT_SEND_STR("resetPD");
  if( (FALSE == ActiveWakeUpState) )
  {
    powerDownTicks = 0;
  }
  ZCB_SetPowerDownTimeout(timeout);
}

PCB(ZCB_SetPowerDownTimeoutWakeUpStateCheck)(BYTE timeout)
{
  ZW_DEBUG_BATT_SEND_STR("SetPDWUSC ");
  ZW_DEBUG_BATT_SEND_NUM(timeout);
  if( (FALSE == ActiveWakeUpState) || (timeout > SEC_10_POWERDOWNTIMEOUT))
  {
    ZCB_ClearSetPowerDownTimeout(timeout);
  }
  else
  {
    ZCB_ClearSetPowerDownTimeout(SEC_10_POWERDOWNTIMEOUT);
  }
}

void
PowerDownNow(void)
{
  ZW_DEBUG_BATT_SEND_STR("PowDN");
  /*Stop the power down timer*/
   ZCB_StopPowerDownTimer();
  if( BATT_MODE_NOT_LISTENING == batteryMode)
  {
    if(0 == sleepPeriod )
    {
      /*
      No minimum / maximum / default wake up interval, the battery-operated
      device is activated by e.g. user interaction in form of a button press on
      the battery-operated device. If this field is 0, then all the other fields
       MUST also be 0.
      Note: This is identical to the specified behavior in Version 1 of the
      command class.
      */

      ZW_DEBUG_BATT_SEND_STR("ZW_STOP_MODE");
      if(!ZW_SetSleepMode(ZW_STOP_MODE,ZW_INT_MASK_EXT1,0))
      {
        ZW_DEBUG_BATT_SEND_STR("ZW_SetSleepMode FAILED");
        ZCB_ResetPowerDownTimeout(0);
      }
    }
    else
    {
      /*WUT timer sleep*/
      if(0 != wakeupCount)
      {
        ZW_DEBUG_BATT_SEND_STR("MAX_V_T");
        ZW_SetWutTimeout(MAX_WUT_TIME - 1); /*drift over 1.38 seconds -> decrease wich 1 seconds*/
      }
      else
      {
        ZW_DEBUG_BATT_SEND_STR("sSRT");
        ZW_DEBUG_BATT_SEND_NUM(sleepStepRestTime);
        ZW_SetWutTimeout(sleepStepRestTime);
      }
      if (!ZW_SetSleepMode(ZW_WUT_MODE,ZW_INT_MASK_EXT1,0))
      {
        ZCB_ResetPowerDownTimeout(0);
      }
    }
  }
  else if(BATT_MODE_LISTENING == batteryMode)
  {
    /* Goto sleep */
    ZW_DEBUG_BATT_SEND_STR("ZW_FREQ_LIST_MODE");

    if (!ZW_SetSleepMode(ZW_FREQUENTLY_LISTENING_MODE, ZW_INT_MASK_EXT1, 0))
    {
      /* Could not sleep now, retrying later*/
      ZW_DEBUG_BATT_SEND_BYTE('r');
      ZCB_ResetPowerDownTimeout(0);
    }

  }
  else
  {
    ZW_DEBUG_BATT_SEND_STR("Wrong mode!");
  }
  ZW_DEBUG_BATT_SEND_NL();
}

BOOL
BatteryInit(
    BATT_MODE mode,
    BYTE wakeUpReason)
{
  ZW_DEBUG_BATT_SEND_STR("UWC");
  ZW_DEBUG_BATT_SEND_NUM(wakeupCount);
  ZW_DEBUG_BATT_SEND_NL();

  batteryMode = mode;
  ActiveWakeUpState = FALSE;

  ZCB_StartPowerDownTimer();

  if(SW_WAKEUP_SENSOR == wakeUpReason)
  {
    ZCB_SetPowerDownTimeout(SEC_2_POWERDOWNTIMEOUT);
  }
  else
  {
    ZCB_SetPowerDownTimeout(MSEC_200_POWERDOWNTIMEOUT);
  }

  if (ZW_WAKEUP_WUT == wakeUpReason) /* 0 - Reset, 1 - WUT, 2 - Wakeup Beam */
  {
    if (0 == wakeupCount)
    {
      wakeupCount = sleepStepTime;
      return TRUE; /* Wake Up */
    }
    else
    {
      wakeupCount--;
    }
  }
  else if((ZW_WAKEUP_EXT_INT == wakeUpReason) ||
          (ZW_WAKEUP_RESET == wakeUpReason)   ||
          (ZW_WAKEUP_SENSOR == wakeUpReason)  ||
          (ZW_WAKEUP_POR == wakeUpReason)  ||
          (ZW_WAKEUP_USB_SUSPEND == wakeUpReason))
  {
    wakeupCount = sleepStepTime;
    return TRUE; /* Wake Up */
  }
  else
  {
    wakeupCount = sleepStepTime;;  /*Initialize wakeupCount*/
  }

  PowerDownNow();
  return FALSE;
}

#ifndef FLIRS
void
SetDefaultBatteryConfiguration(uint32_t sleep)
{
  sleepPeriod = sleep;
  CalcStepTimeRestTime();
  //MemoryPutBuffer((WORD)&EEOFFSET_SLEEP_PERIOD_far, (BYTE_P)&sleepPeriod, sizeof(DWORD), NULL);
  ZW_MemoryPutBuffer((WORD)&EEOFFSET_SLEEP_PERIOD_far, (BYTE_P)&sleepPeriod, sizeof(uint32_t));
  wakeupCount = sleepStepTime;
}

void
LoadBatteryConfiguration(void)
{
  MemoryGetBuffer((WORD)&EEOFFSET_SLEEP_PERIOD_far, (BYTE_P)&sleepPeriod, sizeof(DWORD));
  CalcStepTimeRestTime();
}

/**
 * @brief Calculate sleepStepTime and sleepStepRestTime parameters.
 */
static void
CalcStepTimeRestTime(void)
{
  if( MAX_WUT_TIME < sleepPeriod) /* sleepPeriod is more than 255 sec.*/
  {
    sleepStepTime = sleepPeriod / MAX_WUT_TIME;
    sleepStepRestTime = sleepPeriod % MAX_WUT_TIME;
  }
  else
  {
    sleepStepTime = 0;
    sleepStepRestTime = sleepPeriod;
  }
}
#endif

PCB(ZCB_WakeUpStateSet)(BYTE active)
{
  ZW_DEBUG_BATT_SEND_STR("*** ZCB_WakeUpStateSet ");
  ZW_DEBUG_BATT_SEND_NUM(active);
  ZW_DEBUG_BATT_SEND_NL();
  if(TRUE == active)
  {
    ActiveWakeUpState = TRUE;
    /*We timeout for 10 seconds before going to sleep*/
    ZCB_SetPowerDownTimeout(SEC_10_POWERDOWNTIMEOUT);
  }
  else
  {
    ActiveWakeUpState = FALSE;
  }
}
