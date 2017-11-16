/***************************************************************************
*
* Copyright (c) 2001-2014
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Some nice descriptive description.
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2014/08/07 10:30:17 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <notification.h>
#include <CommandClassNotification.h>
#include "config_app.h"
#include "eeprom.h"
#include <ZW_uart_api.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_NOTIFICATION
#define ZW_DEBUG_NOTIFICATION_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_NOTIFICATION_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_NOTIFICATION_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_NOTIFICATION_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_NOTIFICATION_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_NOTIFICATION_SEND_BYTE(data)
#define ZW_DEBUG_NOTIFICATION_SEND_STR(STR)
#define ZW_DEBUG_NOTIFICATION_SEND_NUM(data)
#define ZW_DEBUG_NOTIFICATION_SEND_WORD_NUM(data)
#define ZW_DEBUG_NOTIFICATION_SEND_NL()
#endif

typedef struct _NOTIFICATION_
{
  NOTIFICATION_TYPE type;
  BYTE event;
  BYTE* pEvPar;
  BYTE evParLen;
} NOTIFICATION;

typedef struct _MY_NOTIFICATION_
{
  BYTE lastActionGrp;
  BYTE lastNotificationType;
  NOTIFICATION grp[MAX_NOTIFICATIONS];
} MY_NOTIFICATION;
/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

MY_NOTIFICATION myNotification;

BOOL notificationBurglerUnknownEvent = FALSE;
BYTE notificationBurglerSequenceNbr = 0;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
BOOL ValidateNotificationType(NOTIFICATION_TYPE notificationType);


/*============================ InitNotification ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
InitNotification(void)
{
  BYTE i = 0;
  notificationBurglerUnknownEvent = FALSE;

  for(i = 0; i< MAX_NOTIFICATIONS; i++)
  {
    myNotification.grp[i].type = NOTIFICATION_TYPE_NONE;
    myNotification.grp[i].event = 0;
  }
  myNotification.lastActionGrp = 0xff;
  myNotification.lastNotificationType = NOTIFICATION_TYPE_NONE;

}


/*============================ AddNotification ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
BOOL
AddNotification(NOTIFICATION_TYPE type, BYTE event, BYTE* pEvPar, BYTE evParLen)
{
  BYTE i;
  /*Find free slot*/
  for(i = 0; i< MAX_NOTIFICATIONS; i++)
  {
    if( 0 == myNotification.grp[i].type)
    {
      myNotification.lastNotificationType = type;
      myNotification.grp[i].type = type;
      myNotification.grp[i].event = event;
      myNotification.grp[i].pEvPar = pEvPar;
      myNotification.grp[i].evParLen = evParLen;
      return TRUE;
    }
  }
  return FALSE;
}


/*============================ GetGroupNotificationType ===================
** Function description
** This function...
**-------------------------------------------------------------------------*/
BYTE
GetGroupNotificationType(NOTIFICATION_TYPE* pNotificationType)
{
  BYTE i = 0;

  ZW_DEBUG_NOTIFICATION_SEND_STR("GetGroupNotificationType ");
  ZW_DEBUG_NOTIFICATION_SEND_NUM((BYTE)*pNotificationType);
  ZW_DEBUG_NOTIFICATION_SEND_BYTE(' ');

  if(0xFF == *pNotificationType)
  {
    /*Check last action is ready*/
    if(0xff != myNotification.lastActionGrp)
    {
      *pNotificationType =  myNotification.grp[myNotification.lastActionGrp].type;
    }
    else
    {
      /*If no last action, then set it to first notification group*/
      myNotification.lastActionGrp= 0;
      *pNotificationType =  myNotification.grp[ myNotification.lastActionGrp].type;

    }
  }

  for(i = 0; i< MAX_NOTIFICATIONS; i++)
  {
    if(myNotification.grp[i].type == *pNotificationType)
    {
      ZW_DEBUG_NOTIFICATION_SEND_NUM(i);
      ZW_DEBUG_NOTIFICATION_SEND_NL();
      return i;
    }
  }
      ZW_DEBUG_NOTIFICATION_SEND_NUM(0xff);
      ZW_DEBUG_NOTIFICATION_SEND_NL();
  return 0xff;
}


/*========================   handleAppNotificationSet  ===========
**    Application specific Notification Set cmd handler
**
**   Side effects: none
**--------------------------------------------------------------------------*/
void
handleAppNotificationSet(NOTIFICATION_TYPE notificationType, NOTIFICATION_STATUS_SET notificationStatus)
{
  BYTE grp;
  ZW_DEBUG_NOTIFICATION_SEND_STR("handleAppNotificationSet ");
  ZW_DEBUG_NOTIFICATION_SEND_NUM(notificationType);
  ZW_DEBUG_NOTIFICATION_SEND_NUM(notificationStatus);
  ZW_DEBUG_NOTIFICATION_SEND_NL();
  if (TRUE == ValidateNotificationType(notificationType))
  {
    MemoryPutByte((WORD)&(nvmApplDescriptor.alarmStatus_far[GetGroupNotificationType(&notificationType)]), (BYTE)notificationStatus);
  }
}


/*========================   handleCmdClassNotificationEventSupportedReport  ===========
**  The Event Supported Report Command is transmitted as a result of a received
**  Event Supported Get Command and MUST not be sent unsolicited.  If an Event
**  Supported Get is received with a not supported Notification Type or Notification
**  Type = 0xFF, the device MUST respond with Event Supported Report (Notification
**  Type = the requested, number of bit masks = 0).
**  param notificationType
**  param pBitMask bit-mask for application supported Z-Wave Alarm types.
**  param len number of bytes in bitmask.
**  Side effects: none
**--------------------------------------------------------------------------*/
void
handleCmdClassNotificationEventSupportedReport(BYTE notificationType, BYTE* pNbrBitMask, BYTE* pBitMaskArray)
{
  ZW_DEBUG_NOTIFICATION_SEND_STR("handleCmdClassNotificationEventSupportedReport ");
  ZW_DEBUG_NOTIFICATION_SEND_NUM(notificationType);
  ZW_DEBUG_NOTIFICATION_SEND_NL();
  if( TRUE == ValidateNotificationType(notificationType) && (0xff != notificationType))
  {
    *pNbrBitMask = 1 + NOTIFICATION_EVENT_HOME_SECURITY_MOTION_DETECTION_UNKNOWN_LOCATION/8;
    pBitMaskArray[0] = 0;
    pBitMaskArray[1] = 1;
  }
  else{
    /*Only support Unkown event why bit maks is 0*/
    *pNbrBitMask = 0;
  }
}



/*============================ CmdClassNotificationGetNotificationStatus =====
** Function description
** Return  notification status.
**-------------------------------------------------------------------------*/
NOTIFICATION_STATUS
CmdClassNotificationGetNotificationStatus(BYTE notificationType)
{
  NOTIFICATION_STATUS status = NOTIFICATION_STATUS_UNSOLICIT_DEACTIVATED;

  ZW_DEBUG_NOTIFICATION_SEND_STR("CmdClassNotificationGetNotificationStatus ");
  ZW_DEBUG_NOTIFICATION_SEND_NUM(notificationType);
  if (TRUE == ValidateNotificationType(notificationType) )
  {
    if(MemoryGetByte((WORD) &nvmApplDescriptor.alarmStatus_far[GetGroupNotificationType((NOTIFICATION_TYPE*)&notificationType)]))
    {
      status = NOTIFICATION_STATUS_UNSOLICIT_ACTIVED;
    }
  }
  return status;
}


/*============================ CmdClassNotificationGetNotificationEvent =====
** Function description
** This function...
**-------------------------------------------------------------------------*/
BOOL
CmdClassNotificationGetNotificationEvent(BYTE* pNotificationType,
                                         BYTE* pNotificationEvent,
                                         BYTE* pEventPar,
                                         BYTE* pEvNbrs)
{
  BYTE grp_nbr = GetGroupNotificationType((NOTIFICATION_TYPE*)pNotificationType);
  ZW_DEBUG_NOTIFICATION_SEND_STR("CmdClassNotificationGetNotificationEvent ");
  ZW_DEBUG_NOTIFICATION_SEND_NUM(*pNotificationType);
  ZW_DEBUG_NOTIFICATION_SEND_NUM(*pNotificationEvent);
  ZW_DEBUG_NOTIFICATION_SEND_NUM(myNotification.grp[grp_nbr].evParLen);
  ZW_DEBUG_NOTIFICATION_SEND_NL();
  if(TRUE == ValidateNotificationType(*pNotificationType) && 0xff != grp_nbr)
  {
    if((*pNotificationEvent == myNotification.grp[grp_nbr].event) ||
       (0x00 == *pNotificationEvent))
    {
      if( 0xff != myNotification.lastActionGrp)
      {
        BYTE i = 0;
        *pEvNbrs = myNotification.grp[grp_nbr].evParLen;
        for(i = 0; i < myNotification.grp[grp_nbr].evParLen; i++)
        {
          pEventPar[i] = myNotification.grp[grp_nbr].pEvPar[i];
        }

        ZW_DEBUG_NOTIFICATION_SEND_STR(" event");
        ZW_DEBUG_NOTIFICATION_SEND_NL();
        return TRUE;
      }
      else{
        ZW_DEBUG_NOTIFICATION_SEND_STR(" no event 2");
        ZW_DEBUG_NOTIFICATION_SEND_NL();
        *pNotificationType = myNotification.lastNotificationType;
        *pNotificationEvent = 0x00;
        return TRUE;
      }
    }
    ZW_DEBUG_NOTIFICATION_SEND_STR(" unknown event");
    *pNotificationType = myNotification.lastNotificationType;
    *pNotificationEvent = 0xFE;
    *pEventPar = NULL;
    *pEvNbrs = 0;
    ZW_DEBUG_NOTIFICATION_SEND_NL();
    return TRUE;
  }
  ZW_DEBUG_NOTIFICATION_SEND_STR("wrong event");
  ZW_DEBUG_NOTIFICATION_SEND_NL();
  return FALSE;
}


/*============================ NotificationEventTrigger ===============================
** Function description
** Trig an event
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
NotificationEventTrigger(BYTE notificationType,BYTE notificationEvent)
{
  ZW_DEBUG_NOTIFICATION_SEND_STR("NotificationEventTrigger ");
  ZW_DEBUG_NOTIFICATION_SEND_NUM(notificationType);
  ZW_DEBUG_NOTIFICATION_SEND_NUM(notificationEvent);
  ZW_DEBUG_NOTIFICATION_SEND_NL();

  if(TRUE == ValidateNotificationType(notificationType))
  {
    myNotification.lastNotificationType = notificationType;
    if(myNotification.grp[GetGroupNotificationType((NOTIFICATION_TYPE*) &notificationType)].event == notificationEvent)
    {
      myNotification.lastActionGrp = GetGroupNotificationType((NOTIFICATION_TYPE*) &notificationType);
    }
    else
    {
      /*Not legal event*/
      myNotification.lastActionGrp = 0xff;
    }
  }
}


/*============================ ReadLastNotificationAction ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
BOOL
ReadLastNotificationAction(NOTIFICATION_TYPE* pType, BYTE* pEvent)
{
  ZW_DEBUG_NOTIFICATION_SEND_STR("ReadLastNotificationAction ");
  if(0xFF != myNotification.lastActionGrp)
  {
    if( NULL != pType){
      *pType = myNotification.grp[myNotification.lastActionGrp].type;
    }
    if( NULL != pEvent){
      *pEvent = myNotification.grp[myNotification.lastActionGrp].event;
    }
    ZW_DEBUG_NOTIFICATION_SEND_NUM(*pType);
    ZW_DEBUG_NOTIFICATION_SEND_NUM(*pEvent);
    ZW_DEBUG_NOTIFICATION_SEND_NL();
    return TRUE;
  }
  ZW_DEBUG_NOTIFICATION_SEND_NL();
  return FALSE;
}

/*============================ ClearLatNotificationAction ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
ClearLastNotificationAction(void)
{
  ZW_DEBUG_NOTIFICATION_SEND_STR("ClearLastNotificationAction ");
  ZW_DEBUG_NOTIFICATION_SEND_NL();
  myNotification.lastActionGrp = 0xff;
}


/*============================ handleCmdClassNotificationSupportedReport ======
** Function description
** Report the supported Notification Types in the application in a Bit Mask array.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
handleCmdClassNotificationSupportedReport( BYTE* pNbrBitMask, BYTE* pBitMaskArray)
{
  BYTE i = 0;
  *pNbrBitMask = 0;

  ZW_DEBUG_NOTIFICATION_SEND_STR("handleCmdClassNotificationSupportedReport ");
  ZW_DEBUG_NOTIFICATION_SEND_NUM(*pNbrBitMask);
  ZW_DEBUG_NOTIFICATION_SEND_NUM(*pBitMaskArray);
  ZW_DEBUG_NOTIFICATION_SEND_NL();
  for(i = 0; i< MAX_NOTIFICATIONS; i++)
  {
    if( myNotification.grp[i].type != NOTIFICATION_TYPE_NONE)
    {
      /* Find max number of bit masks*/
      if(*pNbrBitMask < ((myNotification.grp[i].type / 8) + 1))
      {
        *pNbrBitMask = (myNotification.grp[i].type / 8) + 1;
      }
      /* Add Bit in bit-mask byte (myNotification.grp[i].type / 8)*/
      *(pBitMaskArray + (myNotification.grp[i].type / 8)) |= (1 << ((myNotification.grp[i].type) % 8));
    }
  }
}

/*============================ ValidateNotificationType ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
BOOL
ValidateNotificationType(NOTIFICATION_TYPE notificationType)
{
  BYTE i = 0;
  ZW_DEBUG_NOTIFICATION_SEND_STR("ValidateNotificationType ");
  ZW_DEBUG_NOTIFICATION_SEND_NUM(notificationType);
  ZW_DEBUG_NOTIFICATION_SEND_BYTE(' ');

  if( 0xFF == notificationType)
  {
    return TRUE;
  }

  for(i = 0; i< MAX_NOTIFICATIONS; i++)
  {
    if(myNotification.grp[i].type == notificationType)
    {
      ZW_DEBUG_NOTIFICATION_SEND_NUM(1);
      ZW_DEBUG_NOTIFICATION_SEND_NL();
      return TRUE;
    }
  }
  ZW_DEBUG_NOTIFICATION_SEND_NUM(0);
  ZW_DEBUG_NOTIFICATION_SEND_NL();
  return FALSE;
}

