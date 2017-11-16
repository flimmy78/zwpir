/**
 * @file
 * Handler for Command Class Notification.
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMAND_CLASS_NOTIFICATION_H_
#define _COMMAND_CLASS_NOTIFICATION_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_sysdefs.h>
#include <ZW_pindefs.h>
#include <ZW_evaldefs.h>
#include <ZW_classcmd.h>
#include <ZW_tx_mutex.h>

/**
 * Returns the version of this CC.
 */
#define CommandClassNotificationVersionGet() NOTIFICATION_VERSION_V4

/**
 * Notification type (8 bit).
 */
typedef enum
{
  NOTIFICATION_TYPE_NONE,
  NOTIFICATION_TYPE_SMOKE_ALARM = (0xFF & ICON_TYPE_SPECIFIC_SENSOR_NOTIFICATION_SMOKE_ALARM),
  NOTIFICATION_TYPE_CO_ALARM = (0xFF & ICON_TYPE_SPECIFIC_SENSOR_NOTIFICATION_CO_ALARM),
  NOTIFICATION_TYPE_CO2_ALARM = (0XFF & ICON_TYPE_SPECIFIC_SENSOR_NOTIFICATION_CO2_ALARM),
  NOTIFICATION_TYPE_HEAT_ALARM = (0XFF & ICON_TYPE_SPECIFIC_SENSOR_NOTIFICATION_HEAT_ALARM),
  NOTIFICATION_TYPE_WATER_ALARM = (0XFF & ICON_TYPE_SPECIFIC_SENSOR_NOTIFICATION_WATER_ALARM),
  NOTIFICATION_TYPE_AcCESS_CONTROL = (0XFF & ICON_TYPE_SPECIFIC_SENSOR_NOTIFICATION_ACCESS_CONTROL),
  NOTIFICATION_TYPE_HOME_SECURITY = (0xFF & ICON_TYPE_SPECIFIC_SENSOR_NOTIFICATION_HOME_SECURITY),
  NOTIFICATION_TYPE_POWER_MANAGEMENT = (0XFF & ICON_TYPE_SPECIFIC_SENSOR_NOTIFICATION_POWER_MANAGEMENT),
  NOTIFICATION_TYPE_SYSTEM = (0XFF & ICON_TYPE_SPECIFIC_SENSOR_NOTIFICATION_SYSTEM),
  NOTIFICATION_TYPE_EMERGENCY_ALARM = (0XFF & ICON_TYPE_SPECIFIC_SENSOR_NOTIFICATION_EMERGENCY_ALARM),
  NOTIFICATION_TYPE_CLOCK = (0XFF & ICON_TYPE_SPECIFIC_SENSOR_NOTIFICATION_CLOCK),
  NOTIFICATION_TYPE_MULTIDEVICE = (0XFF & ICON_TYPE_SPECIFIC_SENSOR_NOTIFICATION_MULTIDEVICE)
} NOTIFICATION_TYPE;


/**
 * Notification event (8 bit) for notification type NOTIFICATION_TYPE_HOME_SECURITY.
 */
typedef enum
{
  NOTIFICATION_EVENT_HOME_SECURITY_NO_EVENT,
  NOTIFICATION_EVENT_HOME_SECURITY_INTRUSION,
  NOTIFICATION_EVENT_HOME_SECURITY_INTRUSION_UNKNOWN_EV,
  NOTIFICATION_EVENT_HOME_SECURITY_TAMPERING_COVERING_REMOVED,
  NOTIFICATION_EVENT_HOME_SECURITY_TAMPERING_INVALID_CODE,
  NOTIFICATION_EVENT_HOME_SECURITY_GLASS_BREAKAGE,
  NOTIFICATION_EVENT_HOME_SECURITY_GLASS_BREAKAGE_UNKNOWN_LOCATION,
  NOTIFICATION_EVENT_HOME_SECURITY_MOTION_DETECTION,
  NOTIFICATION_EVENT_HOME_SECURITY_MOTION_DETECTION_UNKNOWN_LOCATION,
  NOTIFICATION_EVENT_HOME_SECURITY_UNKNOWN_EVENT = 0xFE
}NOTIFICATION_EVENT_HOME_SECURITY;

/**
 * Notification event (8 bit) for notification type NOTIFICATION_TYPE_POWER_MANAGEMENT.
 */
typedef enum
{
  NOTIFICATION_EVENT_POWER_MANAGEMENT_NO_EVENT,
  NOTIFICATION_EVENT_POWER_MANAGEMENT_POWER_HAS_BEEN_APPLIED,
  NOTIFICATION_EVENT_POWER_MANAGEMENT_AC_MAINS_DISCONNECED,
  NOTIFICATION_EVENT_POWER_MANAGEMENT_AC_MAINS_RECONNECED,
  NOTIFICATION_EVENT_POWER_MANAGEMENT_SURGE_DETECTED,
  NOTIFICATION_EVENT_POWER_MANAGEMENT_VOLTAGE_DROP_DRIFT,
  NOTIFICATION_EVENT_POWER_MANAGEMENT_OVERCURRENT_DETECTED,
  NOTIFICATION_EVENT_POWER_MANAGEMENT_OVERVOLTAGE_DETECTION,
  NOTIFICATION_EVENT_POWER_MANAGEMENT_OVERLOADED_DETECTED,
  NOTIFICATION_EVENT_POWER_MANAGEMENT_LOAD_ERROR,
  NOTIFICATION_EVENT_POWER_MANAGEMENT_REPLACE_BATTERY_SOON,
  NOTIFICATION_EVENT_POWER_MANAGEMENT_REPLACE_BATTERY_NOW,
  NOTIFICATION_EVENT_POWER_MANAGEMENT_BATTERY_IS_CHARGING,
  NOTIFICATION_EVENT_POWER_MANAGEMENT_BATTERY_IS_FULLY_CHARGED,
  NOTIFICATION_EVENT_POWER_MANAGEMENT_CHARGE_BATTERY_SOON,
  NOTIFICATION_EVENT_POWER_MANAGEMENT_CHARGE_BATTERY_NOW,
  NOTIFICATION_EVENT_POWER_MANAGEMENT_UNKNOWN_EVENT = 0xFE
}NOTIFICATION_EVENT_POWER_MANAGEMENT;


/**
 * Notification event (8 bit) for notification type NOTIFICATION_TYPE_SMOKE_ALARM.
 */
typedef enum
{
  NOTIFICATION_EVENT_EMERGENCY_ALARM_NO_EVENT,
  NOTIFICATION_EVENT_EMERGENCY_ALARM_CONTACT_POLICE,
  NOTIFICATION_EVENT_EMERGENCY_ALARM_CONTACT_FIRE_SERVICE,
  NOTIFICATION_EVENT_EMERGENCY_ALARM_CONTACT_MEDICAL_SERVICE,
  NOTIFICATION_EVENT_EMERGENCY_UNKNOWN_EVENT = 0xFE
}NOTIFICATION_EVENT_EMERGENCY_ALARM;


/**
 * Notification event (8 bit) for notification type NOTIFICATION_TYPE_SYSTEM.
 */
typedef enum
{
  NOTIFICATION_EVENT_SYSTEM_NO_EVENT,
  NOTIFICATION_EVENT_SYSTEM_HARDWARE_FAILURE,
  NOTIFICATION_EVENT_SYSTEM_SOFTWARE_FAILURE,
  NOTIFICATION_EVENT_SYSTEM_HARDWARE_FAILURE_WITH_MANUFACTURER_PROPRIETARY_FAILURE_CODE,
  NOTIFICATION_EVENT_SYSTEM_SOFTWARE_FAILURE_WITH_MANUFACTURER_PROPRIETARY_FAILURE_CODE,
  NOTIFICATION_EVENT_SYSTEM_UNKNOWN_EVENT = 0xFE
}NOTIFICATION_EVENT_SYSTEM;



/**
 * Notification event (8 bit) for notification type NOTIFICATION_TYPE_SMOKE_ALARM.
 */
typedef enum
{
  NOTIFICATION_EVENT_SMOKE_ALARM_NO_EVENT,
  NOTIFICATION_EVENT_SMOKE_ALARM_SMOKE_DETECTED,
  NOTIFICATION_EVENT_SMOKE_ALARM_SMODE_DETECTED_UNKNOWN_LOCATION,
  NOTIFICATION_EVENT_SMOKE_ALARM_TEST,
  NOTIFICATION_EVENT_SMOKE_ALARM_REPLACEMENT_REQUIRED,
  NOTIFICATION_EVENT_SMOKE_ALARM_UNKNOWN_EVENT = 0xFE
}NOTIFICATION_EVENT_SMOKE_ALARM;

/**
 * Notification status unsolicited set
 */
typedef enum
{
  NOTIFICATION_STATUS_SET_UNSOLICIT_DEACTIVATED = 0x00,
  NOTIFICATION_STATUS_SET_UNSOLICIT_ACTIVED = 0xFF
} NOTIFICATION_STATUS_SET;

typedef enum
{
  NOTIFICATION_STATUS_UNSOLICIT_DEACTIVATED = 0x00,
  NOTIFICATION_STATUS_NO_PENDING_NOTIFICATION = 0xFE,
  NOTIFICATION_STATUS_UNSOLICIT_ACTIVED = 0xFF
} NOTIFICATION_STATUS;



/**
 * @brief handleAppNotificationSet
 * Application specific Notification Set cmd handler
 * @param par description..
 * @return description..
 */
extern void
handleAppNotificationSet(
  NOTIFICATION_TYPE notificationType,
  NOTIFICATION_STATUS_SET notificationStatus);


/**
 * @brief CmdClassNotificationGetNotification
 *  User application function. The Notification Status identifier can be set to the
 *  following values:
 *  Notification Status |	Description
 *  --------------------|-------------------------------------------------------------
 *  0x00                | Unsolicited notification is deactivated. The group mapped to Notification Command Class in Association Command Class is not configured with any node IDs.
 *  0xFF                | Unsolicited notification is activated or a pending notification is present.
 * @param notificationType 8 bit type. If 0xFF, return first detected Notification on supported list.
 * @return 8 bit notification status.
 */
extern NOTIFICATION_STATUS
CmdClassNotificationGetNotificationStatus(BYTE notificationType);


/**
 * @brief CmdClassNotificationGetNotificationEvent
 * User application function. Read event.
 * Event Parameter 1 … Event Parameter N (N * Bytes).
 * If the “Event Parameters Length” field is not equal to ‘0’, these field(s)
 * contains the encapsulated information available to the “Notification Type”
 * and “Event”.
 * @param pNotificationType pointer to 8 bit type. If 0xFF, return first detected Notification on supported list.
 * @param pNotificationEvent pointer to 8 bit event.
 * @param pEventPar pointer to Event Parameter 1 … Event Parameter N .
 * @param pEvNbrs pointer to number of parameters N.
 * @return if event is legal.
 */
extern BOOL
CmdClassNotificationGetNotificationEvent(BYTE* pNotificationType,
                                        BYTE* pNotificationEvent,
                                        BYTE* pEventPar,
                                        BYTE* pEvNbrs);


/**
 * @brief CmdClassNotificationGetType
 * User application function. See SDS11060.doc table "4.73.3.1	Table of defined
 * Notification Types & Events".
 * @return 8 bit type.
 */
extern BYTE
CmdClassNotificationGetType(void);

/**
 * @brief CmdClassNotificationGetEvent
 * User application function. See SDS11060.doc table "4.73.3.1	Table of defined
 * Notification Types & Events".
 * @return 8 bit event.
 */
extern BYTE
CmdClassNotificationGetEvent(void);


/**
 * @brief handleCmdClassNotificationSupportedReport
 * The Notification Supported Report Command is used to report the supported
 * Notification Types in the application. The Notification Supported Report Command
 * is transmitted as a result of a received Notification Supported Get Command and
 *  MUST not be sent unsolicited.
 * @param pNbrBitMask Indicates the Number of Bit Masks fields (1-31) used
 * in bytes
 * @param pBitMaskArray The Bit Mask fields describe the supported Notification
 * Type(s) by the device
 */
extern void
handleCmdClassNotificationSupportedReport( BYTE* pNbrBitMask, BYTE* pBitMaskArray);


/**
 * @brief handleAppNotificationEventSupportedReport
 *  The Event Supported Report Command is transmitted as a result of a received
 *  Event Supported Get Command and MUST not be sent unsolicited.  If an Event
 *  Supported Get is received with a not supported Notification Type or Notification
 *  @param notificationType notification Type.
 *  @param pNbrBitMask Indicates the Number of Bit Masks fields (1-31) used in bytes.
 *  @param pBitMaskArray The Bit Mask fields describe the supported Events within the requested Notification Type.
Example if Notification Type = Heat Alarm (0x04):

 *  Side effects: none
 * @return description..
 */
extern void
handleCmdClassNotificationEventSupportedReport(BYTE notificationType, BYTE* pNbrBitMask, BYTE* pBitMaskArray);

/*==============================   handleCommandClassNotification  ============
**
**  Function:  handler for Notification CC
**
**  Side effects: None
**
**--------------------------------------------------------------------------*/
extern void
handleCommandClassNotification(
  BYTE  option,                 /* IN Frame header info */
  BYTE  sourceNode,               /* IN Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, the union */
  /*    should be used to access the fields */
  BYTE   cmdLength                /* IN Number of command bytes including the command */
);

/*===============================  NotificationSendStatus  ==============================
**
**
**
**-------------------------------------------------------------------------------------------*/
void NotificationSendStatus (BYTE bStatus, /*IN Status of the notification frame send process*/
                             BYTE bGroupID, /*IN the group ID of the node the notification frame is send to it*/
                             BYTE bNodeID   /*IN the node ID that the notification frame is sendt to it*/
                            );

/*========================   startSendNoficationToAssociationGroup  ==========
**   checks condition for Sending Nofication Unsolicited Frame
**   return boolean
**
**   Side effects: none
**--------------------------------------------------------------------------*/
extern BOOL
startSendNoficationToAssociationGroup(void);



/**
 * @brief CmdClassNotificationReport
 * Send notification report. See SDS11060.doc “Table of defined Notification
 * Types & Events”.
 * @param nodeId destination node.
 * @param notificationType Notification Type (8 bit)
 * @param notificationEvent Event (8 bit)
 * @param completedFunc callback function returning state on job
 * @return description..
 */
JOB_STATUS
CmdClassNotificationReport(BYTE nodeId,
                           BYTE notificationType,
                           BYTE notificationEvent,
                           VOID_CALLBACKFUNC(completedFunc)(BYTE));

/**
 * @brief GetGroupNotificationType
 * Comment function...
 * @param par description..
 * @return description..
 */
extern BYTE GetGroupNotificationType(NOTIFICATION_TYPE* pNotificationType);


#endif /*_COMMAND_CLASS_NOTIFICATION_H_*/
