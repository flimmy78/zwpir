/**
 * @file CommandClassWakeUp.c
 * @brief Wake Up CC.
 * @author Thomas Roll
 * @author Christian Salmony Olsen
 * @copyright Copyright (c) 2001-2016
 * Sigma Designs, Inc.
 * All Rights Reserved
 * @details Handles commands in the Wake Up CC.
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_stdint.h>
#include <ZW_typedefs.h>
#include <CommandClass.h>
#include <misc.h>
#include <eeprom.h>
#include <ZW_mem_api.h>
#include <ZW_TransportSecProtocol.h>
#include <CommandClassWakeUp.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

static uint32_t wakeUpSettings[WAKEUP_PAR_COUNT];

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

void                   /*RET  Nothing       */
ZCB_WakeUpNotificationCallback( TRANSMISSION_RESULT * pTransmissionResult );

/*============================   WakeupNotificationCallback   ======================
**    Callback function for sending wakeup notification
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
PCB(ZCB_WakeUpNotificationCallback)( TRANSMISSION_RESULT * pTransmissionResult )
{
  if (TRANSMIT_COMPLETE_OK != pTransmissionResult->status)
  {
    /* We did not get in contact with the Wakeup Node, dont expect no more information frame */
    ZCB_WakeUpStateSet(FALSE);
  }
}

void
WakeUpNotification(void)
{
  /* Only send wakeup notifiers when sensor is node in a network */
  /* and a recovery operation is not in progress */
  if ( MemoryGetByte((WORD)&EEOFFSET_MASTER_NODEID_far) != 0 )
  {
    ZCB_WakeUpStateSet(TRUE);
    if(JOB_STATUS_BUSY == CmdClassWakeupNotification( ZCB_WakeUpNotificationCallback))
    {
      TRANSMISSION_RESULT transmissionResult;
      transmissionResult.status = JOB_STATUS_BUSY;
      ZCB_WakeUpNotificationCallback(&transmissionResult);
    }
  }
  else
  {
    /* We are not in any network, go idle */
    ZCB_WakeUpStateSet(FALSE);
  }
}


/*============================   WakeUpNotification   ======================
**    Function sends off the Wakeup notification command
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
JOB_STATUS
CmdClassWakeupNotification(
    VOID_CALLBACKFUNC(pCallback)(TRANSMISSION_RESULT * pTransmissionResult))
{
  ZW_APPLICATION_TX_BUFFER *pTxBuf = GetRequestBuffer(pCallback);
  MULTICHAN_NODE_ID masterNode;
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX txOptionsEx;
 if( IS_NULL( pTxBuf ) )
  {
    /*Ongoing job is active.. just stop current job*/
    return JOB_STATUS_BUSY;
  }

  masterNode.node.nodeId = (MemoryGetByte((WORD)&EEOFFSET_MASTER_NODEID_far) == 0xFF ? NODE_BROADCAST : MemoryGetByte((WORD)&EEOFFSET_MASTER_NODEID_far));
  masterNode.node.endpoint = 0;
  masterNode.node.BitAddress = 0;
  masterNode.nodeInfo.BitMultiChannelEncap = 0;
  masterNode.nodeInfo.security = GetHighestSecureLevel(ZW_GetSecurityKeys());
  txOptionsEx.txOptions = (MemoryGetByte((WORD)&EEOFFSET_MASTER_NODEID_far) != 0xFF ? (ZWAVE_PLUS_TX_OPTIONS) : 0);
  txOptionsEx.sourceEndpoint = 0;
  txOptionsEx.pDestNode = &masterNode;

  pTxBuf->ZW_WakeUpNotificationV2Frame.cmdClass = COMMAND_CLASS_WAKE_UP;
  pTxBuf->ZW_WakeUpNotificationV2Frame.cmd = WAKE_UP_NOTIFICATION_V2;

  if (ZW_TX_IN_PROGRESS != Transport_SendRequestEP((BYTE*) pTxBuf,
                                sizeof(ZW_WAKE_UP_NOTIFICATION_FRAME),
                                &txOptionsEx,
                                ZCB_RequestJobStatus))
  {
    /*Free transmit-buffer mutex*/
    FreeRequestBuffer();
    return JOB_STATUS_BUSY;
  }
  return JOB_STATUS_SUCCESS;
}

void
SetWakeUpConfiguration(WAKEUP_PAR type, uint32_t time)
{
  if (type >= WAKEUP_PAR_COUNT)
  {
    return;
  }

  wakeUpSettings[type] = time;
}

received_frame_status_t
HandleCommandClassWakeUp(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength)
{
  ZW_APPLICATION_TX_BUFFER * pTxBuf;
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX * pTxOptionsEx;

  UNUSED(cmdLength);

  if(TRUE == Check_not_legal_response_job(rxOpt))
  {
    // The Wake Up CC does not support endpoint bit addressing.
    return RECEIVED_FRAME_STATUS_FAIL;
  }

  switch(pCmd->ZW_Common.cmd)
  {
    case  WAKE_UP_INTERVAL_SET_V2:
      {
        uint32_t sleepPeriod = 0;

        // Always AND with FF the right place... Then we don't care whether 1's are shifted in.
        sleepPeriod |= (((uint32_t)pCmd->ZW_WakeUpIntervalSetV2Frame.seconds1) << 16) & 0x00FF0000;
        sleepPeriod |= (((uint32_t)pCmd->ZW_WakeUpIntervalSetV2Frame.seconds2) << 8)  & 0x0000FF00;
        sleepPeriod |= (((uint32_t)pCmd->ZW_WakeUpIntervalSetV2Frame.seconds3) << 0)  & 0x000000FF;

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
        MemoryPutByte((WORD)&EEOFFSET_MASTER_NODEID_far, pCmd->ZW_WakeUpIntervalSetV2Frame.nodeid);
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;
    case WAKE_UP_INTERVAL_GET_V2:
      {
        uint32_t sleepPeriod = handleWakeUpIntervalGet();

        pTxBuf = GetResponseBuffer();
        if (IS_NULL(pTxBuf))
        {
          return RECEIVED_FRAME_STATUS_FAIL;
        }

        RxToTxOptions(rxOpt, &pTxOptionsEx);

        pTxBuf->ZW_WakeUpIntervalReportV2Frame.cmdClass = pCmd->ZW_Common.cmdClass;
        pTxBuf->ZW_WakeUpIntervalReportV2Frame.cmd = WAKE_UP_INTERVAL_REPORT_V2;

        pTxBuf->ZW_WakeUpIntervalReportV2Frame.seconds1 = (uint8_t)(sleepPeriod >> 16); // MSB
        pTxBuf->ZW_WakeUpIntervalReportV2Frame.seconds2 = (uint8_t)(sleepPeriod >> 8);
        pTxBuf->ZW_WakeUpIntervalReportV2Frame.seconds3 = (uint8_t)(sleepPeriod >> 0); // LSB

        pTxBuf->ZW_WakeUpIntervalReportV2Frame.nodeid = MemoryGetByte((WORD)&EEOFFSET_MASTER_NODEID_far);

        if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
            (BYTE *)pTxBuf,
            sizeof(ZW_WAKE_UP_INTERVAL_REPORT_FRAME),
            pTxOptionsEx,
            ZCB_ResponseJobStatus))
        {
          /*Free transmit-buffer mutex*/
          FreeResponseBuffer();
        }
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      break;
    case WAKE_UP_NO_MORE_INFORMATION_V2:
      ZCB_WakeUpStateSet(FALSE);
      handleWakeupNoMoreInfo();
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;
    case WAKE_UP_INTERVAL_CAPABILITIES_GET_V2:
      pTxBuf = GetResponseBuffer();
      if (IS_NULL(pTxBuf))
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      RxToTxOptions(rxOpt, &pTxOptionsEx);

      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.cmdClass = pCmd->ZW_Common.cmdClass;
      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.cmd = WAKE_UP_INTERVAL_CAPABILITIES_REPORT_V2;

      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.minimumWakeUpIntervalSeconds1 = (uint8_t)(wakeUpSettings[WAKEUP_PAR_MIN_SLEEP_TIME] >> 16);
      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.minimumWakeUpIntervalSeconds2 = (uint8_t)(wakeUpSettings[WAKEUP_PAR_MIN_SLEEP_TIME] >> 8);
      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.minimumWakeUpIntervalSeconds3 = (uint8_t)(wakeUpSettings[WAKEUP_PAR_MIN_SLEEP_TIME] >> 0);

      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.maximumWakeUpIntervalSeconds1 = (uint8_t)(wakeUpSettings[WAKEUP_PAR_MAX_SLEEP_TIME] >> 16);
      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.maximumWakeUpIntervalSeconds2 = (uint8_t)(wakeUpSettings[WAKEUP_PAR_MAX_SLEEP_TIME] >> 8);
      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.maximumWakeUpIntervalSeconds3 = (uint8_t)(wakeUpSettings[WAKEUP_PAR_MAX_SLEEP_TIME] >> 0);

      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.defaultWakeUpIntervalSeconds1 = (uint8_t)(wakeUpSettings[WAKEUP_PAR_DEFAULT_SLEEP_TIME] >> 16);
      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.defaultWakeUpIntervalSeconds2 = (uint8_t)(wakeUpSettings[WAKEUP_PAR_DEFAULT_SLEEP_TIME] >> 8);
      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.defaultWakeUpIntervalSeconds3 = (uint8_t)(wakeUpSettings[WAKEUP_PAR_DEFAULT_SLEEP_TIME] >> 0);

      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.wakeUpIntervalStepSeconds1 = (uint8_t)(wakeUpSettings[WAKEUP_PAR_SLEEP_STEP] >> 16);
      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.wakeUpIntervalStepSeconds2 = (uint8_t)(wakeUpSettings[WAKEUP_PAR_SLEEP_STEP] >> 8);
      pTxBuf->ZW_WakeUpIntervalCapabilitiesReportV2Frame.wakeUpIntervalStepSeconds3 = (uint8_t)(wakeUpSettings[WAKEUP_PAR_SLEEP_STEP] >> 0);

      if (ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
          (BYTE *)pTxBuf,
          sizeof(ZW_WAKE_UP_INTERVAL_CAPABILITIES_REPORT_V2_FRAME),
          pTxOptionsEx,
          ZCB_ResponseJobStatus))
      {
        FreeResponseBuffer();
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;
    default:
      // Do nothing.
      break;
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}

void
CmdClassWakeUpNotificationMemorySetDefault(void)
{
  MemoryPutByte((WORD)&EEOFFSET_MASTER_NODEID_far, 0);
}
