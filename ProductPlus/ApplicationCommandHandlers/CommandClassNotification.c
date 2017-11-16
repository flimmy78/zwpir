/***************************************************************************
*
* Copyright (c) 2001-2011
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Notification Command Class source file
*
* Author:
*
* Last Changed By:  $Author:  $
* Revision:         $Revision:  $
* Last Changed:     $Date:  $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <ZW_TransportLayer.h>
#include <ZW_tx_mutex.h>

#include "config_app.h"
#include <CommandClassNotification.h>
#include <association_plus.h>
#include <misc.h>
#include <ZW_uart_api.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_CC_NOTIFICATION
#define ZW_DEBUG_CCN_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_CCN_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_CCN_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_CCN_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_CCN_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_CCN_SEND_BYTE(data)
#define ZW_DEBUG_CCN_SEND_STR(STR)
#define ZW_DEBUG_CCN_SEND_NUM(data)
#define ZW_DEBUG_CCN_SEND_WORD_NUM(data)
#define ZW_DEBUG_CCN_SEND_NL()
#endif

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

/*==============================   handleCommandClassNotification  ============
**
**  Function:  handler for Notification CC
**
**  Side effects: None
**
**--------------------------------------------------------------------------*/
void
handleCommandClassNotification(
  BYTE  option,                    /* IN Frame header info */
  BYTE  sourceNode,                /* IN Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER *pCmd,  /* IN Payload from the received frame, the union */
  /*    should be used to access the fields */
  BYTE   cmdLength                 /* IN Number of command bytes including the command */
)
{
  BYTE size;
  ZW_DEBUG_CCN_SEND_STR("CmdClassAlarm ");
  ZW_DEBUG_CCN_SEND_NUM(pCmd->ZW_Common.cmd);
  ZW_DEBUG_CCN_SEND_NL();
  switch (pCmd->ZW_Common.cmd)
  {
    case NOTIFICATION_SET_V4:
      if( 0x00 == pCmd->ZW_NotificationSetV4Frame.notificationStatus ||
          0xff == pCmd->ZW_NotificationSetV4Frame.notificationStatus)
      {
        handleAppNotificationSet(pCmd->ZW_NotificationSetV4Frame.notificationType,
                                 pCmd->ZW_NotificationSetV4Frame.notificationStatus);
      }
      break;

    case NOTIFICATION_GET_V4:
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if(NULL != pTxBuf)
        {
          ZW_DEBUG_CCN_SEND_STR("ZW_ALARM_GET_V4_FRAME");
          pTxBuf->ZW_NotificationReport1byteV4Frame.cmdClass = COMMAND_CLASS_NOTIFICATION_V4;
          pTxBuf->ZW_NotificationReport1byteV4Frame.cmd = NOTIFICATION_REPORT_V4;
          pTxBuf->ZW_NotificationReport1byteV4Frame.v1AlarmType = 0; /*must be set to 0*/
          pTxBuf->ZW_NotificationReport1byteV4Frame.v1AlarmLevel = 0; /*must be set to 0*/
          pTxBuf->ZW_NotificationReport1byteV4Frame.reserved = 0; /*must be set to 0*/
          pTxBuf->ZW_NotificationReport1byteV4Frame.notificationStatus =
            CmdClassNotificationGetNotificationStatus( pCmd->ZW_NotificationGetV4Frame.notificationType );
          pTxBuf->ZW_NotificationReport1byteV4Frame.properties1 = 0;
          pTxBuf->ZW_NotificationReport1byteV4Frame.notificationType = pCmd->ZW_NotificationGetV4Frame.notificationType;
          pTxBuf->ZW_NotificationReport1byteV4Frame.eventParameter1 = 0;
          pTxBuf->ZW_NotificationReport1byteV4Frame.mevent = 0;
          if(3 == cmdLength)
          {
            pTxBuf->ZW_NotificationReport1byteV4Frame.notificationType = 0;
            size = sizeof(ZW_NOTIFICATION_REPORT_1BYTE_V4_FRAME) - 2; //Remove event-parameter and sequence number
          }
          else if(5 > cmdLength) /*CC  V2*/
          {
            ZW_DEBUG_CCN_SEND_STR("CC V2");
            size = 0;
          }
          else{
            pTxBuf->ZW_NotificationReport1byteV4Frame.mevent = pCmd->ZW_NotificationGetV4Frame.mevent;
            size = 0;
          }



          if( 0xff == pTxBuf->ZW_NotificationReport1byteV4Frame.notificationType && 0x00 == pTxBuf->ZW_NotificationReport1byteV4Frame.mevent)
          {
            /* In response to a Notification Get (Notification Type = 0xFF) , a responding device MUST return
               a pending notification from its internal list (Pull mode). We also do it for Push mode.*/

            BYTE grp = GetGroupNotificationType((NOTIFICATION_TYPE*)&pTxBuf->ZW_NotificationReport1byteV4Frame.notificationType);
            if(0xff == grp)
            {
              //ZW_DEBUG_CCN_SEND_STR(" STATUS 0xFE");
              //pTxBuf->ZW_NotificationReport1byteV4Frame.notificationStatus = 0xFE;
              //size = sizeof(ZW_NOTIFICATION_REPORT_1BYTE_V4_FRAME) - 5;
              FreeResponseBuffer();
              break;
            }
          }

          if(0 == size)
          {
            if(TRUE == CmdClassNotificationGetNotificationEvent( &pTxBuf->ZW_NotificationReport1byteV4Frame.notificationType,
                                                            &pTxBuf->ZW_NotificationReport1byteV4Frame.mevent,
                                                            &(pTxBuf->ZW_NotificationReport1byteV4Frame.eventParameter1),
                                                            &(pTxBuf->ZW_NotificationReport1byteV4Frame.properties1)
                                                            ))
            {
              size = (sizeof(ZW_NOTIFICATION_REPORT_1BYTE_V4_FRAME) - sizeof(BYTE) +
                  (pTxBuf->ZW_NotificationReport1byteV4Frame.properties1 & ALARM_TYPE_SUPPORTED_REPORT_PROPERTIES1_NUMBER_OF_BIT_MASKS_MASK_V2)) -
                  sizeof(BYTE); /* Removed sequence number*/
            }
            else{
              /*We do not support alarmType!*/
              /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
              FreeResponseBuffer();
              break;
            }
          }

          if (size)
          {
            if(FALSE == Transport_SendResponse(
                sourceNode,
                pTxBuf,
                size,
                option,
                ZCB_ResponseJobStatus))
            {
              /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
              FreeResponseBuffer();
            }
          }
          else
          {
            /*Size 0, free transmit-buffer pTxBuf by cleaing mutex */
            FreeResponseBuffer();
          }
        }
        else
        {
          ZW_DEBUG_CCN_SEND_STR("Get response buffer failed!");
          ZW_DEBUG_CCN_SEND_NL();
        }
      }
      break;

    case NOTIFICATION_SUPPORTED_GET_V4:
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if( NON_NULL( pTxBuf ) )
        {
          pTxBuf->ZW_NotificationSupportedReport1byteV4Frame.cmdClass = COMMAND_CLASS_NOTIFICATION_V4;
          pTxBuf->ZW_NotificationSupportedReport1byteV4Frame.cmd = NOTIFICATION_SUPPORTED_REPORT_V4;
          handleCmdClassNotificationSupportedReport(&(pTxBuf->ZW_NotificationSupportedReport1byteV4Frame.properties1),
                                               &(pTxBuf->ZW_NotificationSupportedReport1byteV4Frame.bitMask1));
          pTxBuf->ZW_NotificationSupportedReport1byteV4Frame.properties1 &= 0x7F;/*V1 alarm bit: hardcoded to Z-Wave alliance type*/

          if(FALSE == Transport_SendResponse(sourceNode,
                                           pTxBuf,
                                           sizeof(ZW_NOTIFICATION_SUPPORTED_REPORT_1BYTE_V4_FRAME) - 1 +
                                            pTxBuf->ZW_NotificationSupportedReport1byteV4Frame.properties1,
                                           option, ZCB_ResponseJobStatus))
          {
            /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
            FreeResponseBuffer();
          }
        }
      }
      break;

    case EVENT_SUPPORTED_GET_V4:
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if( NON_NULL( pTxBuf ) )
        {
          BYTE i;
          pTxBuf->ZW_EventSupportedReport1byteV4Frame.cmdClass = COMMAND_CLASS_NOTIFICATION_V4;
          pTxBuf->ZW_EventSupportedReport1byteV4Frame.cmd = EVENT_SUPPORTED_REPORT_V4;
          pTxBuf->ZW_EventSupportedReport1byteV4Frame.notificationType = pCmd->ZW_EventSupportedGetV4Frame.notificationType;
          pTxBuf->ZW_EventSupportedReport1byteV4Frame.properties1 = 0;
          handleCmdClassNotificationEventSupportedReport(
                              pTxBuf->ZW_EventSupportedReport1byteV4Frame.notificationType,
                              &(pTxBuf->ZW_EventSupportedReport1byteV4Frame.properties1),
                              &(pTxBuf->ZW_EventSupportedReport1byteV4Frame.bitMask1));

          pTxBuf->ZW_EventSupportedReport1byteV4Frame.properties1 &= 0x7F;

          if(FALSE == Transport_SendResponse(sourceNode,
                                           pTxBuf,
                                           sizeof(ZW_EVENT_SUPPORTED_REPORT_1BYTE_V4_FRAME) - 1 +
                                           (pTxBuf->ZW_EventSupportedReport1byteV4Frame.properties1 & 0x1F), /*remove reserved bits*/
                                           option, ZCB_ResponseJobStatus))
          {
            /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
            FreeResponseBuffer();
          }
        }
      }
      break;

    default:
      break;
  }
}


/*============================ CmdClassNotificationReport ===================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
JOB_STATUS
CmdClassNotificationReport(BYTE nodeId,
                           BYTE notificationType,
                           BYTE notificationEvent,
                           VOID_CALLBACKFUNC(completedFunc)(BYTE))
{
  if(NOTIFICATION_STATUS_UNSOLICIT_ACTIVED == CmdClassNotificationGetNotificationStatus(notificationType))
  {
    ZW_APPLICATION_TX_BUFFER *pTxBuf = GetRequestBuffer(completedFunc);

    if(NULL == pTxBuf)
    {
      /*Ongoing job is active.. just stop current job*/
      return JOB_STATUS_BUSY;
    }

    pTxBuf->ZW_NotificationReport1byteV4Frame.cmdClass = COMMAND_CLASS_NOTIFICATION_V4;
    pTxBuf->ZW_NotificationReport1byteV4Frame.cmd = NOTIFICATION_REPORT_V4;
    pTxBuf->ZW_NotificationReport1byteV4Frame.v1AlarmType = 0;
    pTxBuf->ZW_NotificationReport1byteV4Frame.v1AlarmLevel = 0;
    pTxBuf->ZW_NotificationReport1byteV4Frame.reserved = 0;
    pTxBuf->ZW_NotificationReport1byteV4Frame.notificationStatus = NOTIFICATION_STATUS_UNSOLICIT_ACTIVED;
    pTxBuf->ZW_NotificationReport1byteV4Frame.notificationType = notificationType;
    pTxBuf->ZW_NotificationReport1byteV4Frame.mevent = notificationEvent;

    CmdClassNotificationGetNotificationEvent( &(pTxBuf->ZW_NotificationReport1byteV4Frame.notificationType),
                                              &(pTxBuf->ZW_NotificationReport1byteV4Frame.mevent),
                                            &(pTxBuf->ZW_NotificationReport1byteV4Frame.eventParameter1),
                                            &(pTxBuf->ZW_NotificationReport1byteV4Frame.properties1));
    pTxBuf->ZW_NotificationReport1byteV4Frame.properties1 &= ALARM_TYPE_SUPPORTED_REPORT_PROPERTIES1_NUMBER_OF_BIT_MASKS_MASK_V2; /*remove sequence number and reserved*/

    if (!Transport_SendRequest(
          nodeId,
          pTxBuf,
          (sizeof(ZW_NOTIFICATION_REPORT_1BYTE_V4_FRAME) - sizeof(BYTE) +
        pTxBuf->ZW_NotificationReport1byteV4Frame.properties1) - sizeof(BYTE),
          ZWAVE_PLUS_TX_OPTIONS,
          ZCB_RequestJobStatus, FALSE))
    {
      ZCB_RequestJobStatus(TRANSMIT_COMPLETE_FAIL);
      return JOB_STATUS_BUSY;
    }
  }
  else{
    return JOB_STATUS_BUSY;
  }
  return JOB_STATUS_SUCCESS;
}
