/**
 *
 * Copyright (c) 2001-2014
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 * @file notification.h
 *
 * @brief Notification V4 module. Current version cannot queue up more events, but
 * it is possible to implement a queue in current design.
 *
 * How to use Notification module:
 *
 * Initialization:
 * 1. Call constructor InitNotification()
 * 2. Add application Notification types and events to the module by calling
 *    AddNotification(...)
 *
 * Trigger event:
 * 1.  First trigger an event on Communication Module by calling
 *     NotificationEventTrigger (..) and then call AGI_NodeIdListInit (..) to
 *     trig profile-event at AGI. 
 * 2. AGI now sends an unsolicited events to all nodes. It has the knowledge Command class and
 *    and command (NextUnsolicitedEvent), but need data and get it by calling
 *    ReadLastNotificationAction(..).
 * 3. Finally notification-type/event must to be cleared from the queue. This
 *    done by calling ClearLastNotificationAction(). ClearLastNotificationAction ()
 *    is called when it's done with the unsolicited event jobs.
 *
 * Author: Thomas Roll
 *
 * Last Changed By: $Author: tro $
 * Revision: $Revision: 0.00 $
 * Last Changed: $Date: 2014/08/07 10:27:00 $
 *
 */

#ifndef _NOTIFICATION_H_
#define _NOTIFICATION_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <CommandClassNotification.h>
/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/



/**
 * @brief InitNotification
 * Init notification module
 */
void InitNotification(void);


/**
 * @brief AddNotification
 * Comment function...
 * @param type is notification type.
 * @param event is notification event dependent og the notification type.
 * @param pEvPar pointer to event parameter list.
 * @param evParLen length of list.
 * @return TRUE if success else FALSE.
 */
BOOL AddNotification(NOTIFICATION_TYPE type, BYTE event, BYTE* pEvPar, BYTE evParLen);

/**
 * @brief NotificationEventTrigger
 * Add event on queue (queue size 1)
 * @param notificationType is notification type
 * @param notificationEvent notification event
 */
void NotificationEventTrigger(BYTE notificationType,BYTE notificationEvent);


/**
 * @brief ReadLastNotificationAction
 * Read last event on queue
 * @param pNotificationType pointer to notification type.
 * @param pNotificationEvent pointer to notification event.
 * @param pEvPar pointer to event parameter list.
 * @param evParLen length of list.
 * @return TRUE if event on queue else FALSE.
 */
BOOL ReadLastNotificationAction(NOTIFICATION_TYPE* pType, BYTE* pEvent);


/**
 * @brief ClearLastNotificationAction
 * Clear event on queue
 */
void ClearLastNotificationAction(void);


#endif /* _NOTIFICATION_H_ */


