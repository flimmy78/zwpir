/**
 *
 * Copyright (c) 2001-2014
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 * @file ev_man.h
 *
 * @brief Some nice descriptive description.
 *
 * Author: Thomas Roll
 *
 * Last Changed By: $Author: tro $
 * Revision: $Revision: 0.00 $
 * Last Changed: $Date: 2014/06/11 13:33:43 $
 *
 */

#ifndef _ev_man_H_
#define _ev_man_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
#define DEFINE_EVENT_KEY_NBR 0x10
#define DEFINE_EVENT_APP_NBR 0x20

typedef enum _EVENT_WAKEUP_
{ 
  /**
   * Woken up by reset or external int
   */
  EVENT_WAKEUP_RESET = ZW_WAKEUP_RESET,
  /**
   * Woken up by the WUT timer
   */
  EVENT_WAKEUP_WUT,
  /**
   * Woken up by a wakeup beam
   */
  EVENT_WAKEUP_SENSOR,
  /**
   * Reset because of a watchdog timeout
   */
  EVENT_WAKEUP_WATCHDOG,
  /**
   * Woken up by external interrupt
   */
  EVENT_WAKEUP_EXT_INT,
  /**
   * Reset by Power on reset circuit
   */
  EVENT_WAKEUP_POR,
  /**
   * Woken up by USB suspend
   */
  EVENT_WAKEUP_USB_SUSPEND,
  /**
   * EVENT_WAKEUP_MAX define the last enum type 
   */
  EVENT_WAKEUP_MAX
} EVENT_WAKEUP;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/



#endif /* _ev_man_H_ */


