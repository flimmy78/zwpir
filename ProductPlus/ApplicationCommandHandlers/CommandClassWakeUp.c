/***************************************************************************
*
* Copyright (c) 2001-2013
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Wake Up commmand Class Version 2
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2013/06/25 14:13:34 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <ZW_tx_mutex.h>
#include <ZW_TransportLayer.h>

#include "config_app.h"
#include <CommandClassWakeUp.h>
#include <ZW_uart_api.h>
#include <battery_plus.h>
#include <misc.h>
#include <eeprom.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

static DWORD wakeUpSettings[WAKEUP_PAR_MAX];

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
void                   /*RET  Nothing       */
ZCB_WakeUpNotificationCallback(
  BYTE txStatus  /*IN   Transmission result        */
);

code const void (code * ZCB_WakeupNotificationCallback_p)(BYTE txStatus) = &ZCB_WakeUpNotificationCallback;
/*============================   WakeupNotificationCallback   ======================
**    Callback function for sending wakeup notification
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void                   /*RET  Nothing       */
ZCB_WakeUpNotificationCallback(
  BYTE txStatus  /*IN   Transmission result        */
)
{
  if(TRANSMIT_COMPLETE_OK != txStatus)
  {
    /* We did not get in contact with the Wakeup Node, dont expect no more information frame */
    ZCB_WakeUpStateSet(FALSE);
  }
}


/*============================   WakeUpNotification   ======================
**    Function sends off the Wakeup notification command
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void                   /*RET  Nothing       */
WakeUpNotification()
{
  /* Only send wakeup notifiers when sensor is node in a network */
  /* and a recovery operation is not in progress */
  if ( MemoryGetByte((WORD)&nvmApplDescriptor.EEOFFSET_MASTER_NODEID_far) != 0 )
  {
    ZCB_WakeUpStateSet(TRUE);
    if(JOB_STATUS_BUSY == CmdClassWakeupNotification( ZCB_WakeUpNotificationCallback))
    {
      ZCB_WakeUpNotificationCallback(TRANSMIT_COMPLETE_FAIL);
    }
  }
  else
  {
    /* We are not in any network, go idle */
    ZCB_WakeUpStateSet(FALSE);
  }
}


/*============================   CmdClassWakeupNotification   ================
**    Function sends off the Wakeup notification command
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
JOB_STATUS
CmdClassWakeupNotification(
  VOID_CALLBACKFUNC(pCbFunc)(BYTE val))
{
  ZW_APPLICATION_TX_BUFFER *pTxBuf = GetRequestBuffer(pCbFunc);

  if(NULL == pTxBuf)
  {
    /*Ongoing job is active.. just stop current job*/
    return JOB_STATUS_BUSY;
  }

  pTxBuf->ZW_WakeUpNotificationV2Frame.cmdClass = COMMAND_CLASS_WAKE_UP;
  pTxBuf->ZW_WakeUpNotificationV2Frame.cmd = WAKE_UP_NOTIFICATION_V2;
  if (!Transport_SendRequest( (MemoryGetByte((WORD)&nvmApplDescriptor.EEOFFSET_MASTER_NODEID_far) == 0xFF ? NODE_BROADCAST : MemoryGetByte((WORD)&nvmApplDescriptor.EEOFFSET_MASTER_NODEID_far)),
                             (BYTE *)pTxBuf, sizeof(ZW_WAKE_UP_NOTIFICATION_FRAME),
                             (MemoryGetByte((WORD)&nvmApplDescriptor.EEOFFSET_MASTER_NODEID_far) != 0xFF ? (ZWAVE_PLUS_TX_OPTIONS) : 0),
                             ZCB_RequestJobStatus,FALSE ))
  {
    /*Free transmit-buffer mutex*/
    FreeRequestBuffer();
    return JOB_STATUS_BUSY;
  }
  return JOB_STATUS_SUCCESS;
}


/*============================   SetWakeUpConfiguration   ======================
**    Function set the wakeup timing configuration values.
**    Sleep step time, default sleep time, max sleep time, and
**    min sleep time. these values are in seconds
**    these values are constants
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void                   /*RET  Nothing       */
SetWakeUpConfiguration(WAKEUP_PAR type, DWORD time)
{
  if (type >= WAKEUP_PAR_MAX)
  {
    return;
  }

  wakeUpSettings[type] = time;



}

/*============================   CmdHandler_COMMAND_CLASS_WAKE_UP   ======================
**
**--------------------------------------------------------------------------*/
void                   /* RET  Nothing      */
HandleCommandClassWakeUp(
  BYTE  option,                    /* IN Frame header info */
  BYTE  sourceNode,                /* IN Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER *pCmd,  /* IN Payload from the received frame, the union */
  /*    should be used to access the fields */
  BYTE   cmdLength                 /* IN Number of command bytes including the command */
)
{
  switch(pCmd->ZW_Common.cmd)
  {
    case  WAKE_UP_INTERVAL_SET_V2:
      {
        DWORD sleepPeriod = 0;
        sleepPeriod |= (((DWORD)pCmd->ZW_WakeUpIntervalSetV2Frame.seconds1) << 16) & 0x00FF0000;
        sleepPeriod |= (((DWORD)pCmd->ZW_WakeUpIntervalSetV2Frame.seconds2) << 8)  & 0x0000FF00;
        sleepPeriod |= (((DWORD)pCmd->ZW_WakeUpIntervalSetV2Frame.seconds3) << 0)  & 0x000000FF;

        /*Calculate correct sleep-period dependent of step resolution*/
        if (0 == sleepPeriod)
        {
          // Do nothing.
        }
        else if (sleepPeriod < wakeUpSettings[WAKEUP_PAR_MIN_SLEEP_TIME])
        {
          // This is actually not part of the CC spec, but hopefully will be in version 3!
          sleepPeriod = wakeUpSettings[WAKEUP_PAR_MIN_SLEEP_TIME];
        }
        else if (sleepPeriod > wakeUpSettings[WAKEUP_PAR_MAX_SLEEP_TIME])
        {
          // This is actually not part of the CC spec, but hopefully will be in version 3!
          sleepPeriod = wakeUpSettings[WAKEUP_PAR_MAX_SLEEP_TIME];
        }
        else
        {
          /**
           * The following formula ensures that the sleep period will always match a valid step
           * value.
           *
           *                       input - min
           * sleep_period = min + ------------- * step
           *                           step
           */
          sleepPeriod = wakeUpSettings[WAKEUP_PAR_MIN_SLEEP_TIME] +
            ((sleepPeriod - wakeUpSettings[WAKEUP_PAR_MIN_SLEEP_TIME]) / wakeUpSettings[WAKEUP_PAR_SLEEP_STEP]) * wakeUpSettings[WAKEUP_PAR_SLEEP_STEP];
        }
        SetDefaultBatteryConfiguration(sleepPeriod);
        MemoryPutByte((WORD)&nvmApplDescriptor.EEOFFSET_MASTER_NODEID_far, pCmd->ZW_WakeUpIntervalSetV2Frame.nodeid);
      }
      break;
    case WAKE_UP_INTERVAL_GET_V2:
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();

        /*Check pTxBuf is free*/
        if(NULL != pTxBuf)
        {
          DWORD sleepPeriod = handleWakeUpIntervalGet();
          pTxBuf->ZW_WakeUpIntervalReportV2Frame.cmdClass = pCmd->ZW_Common.cmdClass;
          pTxBuf->ZW_WakeUpIntervalReportV2Frame.cmd = WAKE_UP_INTERVAL_REPORT_V2;

          pTxBuf->ZW_WakeUpIntervalReportV2Frame.seconds1 = (BYTE)(sleepPeriod >> 16); // MSB
          pTxBuf->ZW_WakeUpIntervalReportV2Frame.seconds2 = (BYTE)(sleepPeriod >> 8);
          pTxBuf->ZW_WakeUpIntervalReportV2Frame.seconds3 = (BYTE)(sleepPeriod >> 0); // LSB

          pTxBuf->ZW_WakeUpIntervalReportV2Frame.nodeid = MemoryGetByte((WORD)&nvmApplDescriptor.EEOFFSET_MASTER_NODEID_far);

          if( FALSE == Transport_SendResponse(sourceNode,
                               (BYTE *)pTxBuf, sizeof(ZW_WAKE_UP_INTERVAL_REPORT_FRAME),
                               option,
                               ZCB_ResponseJobStatus))
          {
            /*Free transmit-buffer mutex*/
            FreeResponseBuffer();
          }
        }
      }
      break;
    case WAKE_UP_NO_MORE_INFORMATION_V2:
      ZCB_WakeUpStateSet(FALSE);
      handleWakeupNoMoreInfo();
      break;
    case WAKE_UP_INTERVAL_CAPABILITIES_GET_V2:
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();

        /*Check pTxBuf is free*/
        if(NULL != pTxBuf)
        {
          pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.cmdClass = pCmd->ZW_Common.cmdClass;
          pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.cmd = WAKE_UP_INTERVAL_CAPABILITIES_REPORT_V2;

      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.minimumWakeUpIntervalSeconds1 = (BYTE)(wakeUpSettings[WAKEUP_PAR_MIN_SLEEP_TIME] >> 16);
      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.minimumWakeUpIntervalSeconds2 = (BYTE)(wakeUpSettings[WAKEUP_PAR_MIN_SLEEP_TIME] >> 8);
      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.minimumWakeUpIntervalSeconds3 = (BYTE)(wakeUpSettings[WAKEUP_PAR_MIN_SLEEP_TIME] >> 0);
      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.maximumWakeUpIntervalSeconds1 = (BYTE)(wakeUpSettings[WAKEUP_PAR_MAX_SLEEP_TIME] >> 16);
      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.maximumWakeUpIntervalSeconds2 = (BYTE)(wakeUpSettings[WAKEUP_PAR_MAX_SLEEP_TIME] >> 8);
      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.maximumWakeUpIntervalSeconds3 = (BYTE)(wakeUpSettings[WAKEUP_PAR_MAX_SLEEP_TIME] >> 0);
      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.defaultWakeUpIntervalSeconds1 = (BYTE)(wakeUpSettings[WAKEUP_PAR_DEFAULT_SLEEP_TIME] >> 16);
      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.defaultWakeUpIntervalSeconds2 = (BYTE)(wakeUpSettings[WAKEUP_PAR_DEFAULT_SLEEP_TIME] >> 8);
      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.defaultWakeUpIntervalSeconds3 = (BYTE)(wakeUpSettings[WAKEUP_PAR_DEFAULT_SLEEP_TIME] >> 0);

      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.wakeUpIntervalStepSeconds1 = (BYTE)(wakeUpSettings[WAKEUP_PAR_SLEEP_STEP] >> 16);
      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.wakeUpIntervalStepSeconds2 = (BYTE)(wakeUpSettings[WAKEUP_PAR_SLEEP_STEP] >> 8);
      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.wakeUpIntervalStepSeconds3 = (BYTE)(wakeUpSettings[WAKEUP_PAR_SLEEP_STEP] >> 0);

          if( FALSE == Transport_SendResponse(sourceNode,
                               (BYTE *)pTxBuf, sizeof(ZW_WAKE_UP_INTERVAL_CAPABILITIES_REPORT_V2_FRAME),
                               option,
                               ZCB_ResponseJobStatus))
          {
            /*Free transmit-buffer mutex*/
            FreeResponseBuffer();
          }
        }
      }

  }


}

/*============================ CmdClassWakeUpNotificationMemorySetDefault ==
** Function description
** Set memory EEOFFSET_MASTER_NODEID_far to 0 (deafult value).
**
**-------------------------------------------------------------------------*/
void
CmdClassWakeUpNotificationMemorySetDefault(void)
{
  MemoryPutByte((WORD)&nvmApplDescriptor.EEOFFSET_MASTER_NODEID_far, 0);
}

