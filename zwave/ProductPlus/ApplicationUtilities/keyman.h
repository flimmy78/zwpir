/***************************************************************************
*
* Copyright (c) 2001-2014
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Key manager is a small tools to scan keys and parse key events 
* to application state event function. It use both modules p_button and 
* one_button.
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2014/06/09 11:51:59 $
*
****************************************************************************/
#ifndef _KEYMAN_H_
#define _KEYMAN_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ev_man.h>
/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
typedef enum _EVENT_KEY_
{
  EVENT_KEY_B0_TRIPLE_PRESS = DEFINE_EVENT_KEY_NBR,
  EVENT_KEY_B0_PRESS,
  EVENT_KEY_B0_RELEASE,
  EVENT_KEY_B0_HELD,
  EVENT_KEY_B0_HELD_10_SEC,
  EVENT_KEY_B1_PRESS,
  EVENT_KEY_B1_RELEASE,
  EVENT_KEY_B2_PRESS,
  EVENT_KEY_B2_RELEASE,
  EVENT_KEY_B3_PRESS,
  EVENT_KEY_B3_RELEASE,
  EVENT_KEY_B4_PRESS,
  EVENT_KEY_B4_RELEASE,
  EVENT_KEY_B1_HELD,
  EVENT_KEY_B2_HELD,
  EVENT_KEY_B3_HELD,
  EVENT_KEY_B4_HELD,
  EVENT_KEY_MAX /**< EVENT_KEY_MAX define the last enum type*/
} EVENT_KEY;


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/


/** 
 * @brief InitKeyManager
 * Comment function...
 * @param pAppStMan is function-pointer to application state machine
 * @param pSetPowerDownTimeout is function-pointer to battery 
 * ZCB_pSetPowerDownTimeout(BYTE)
 */
void InitKeyManager( VOID_CALLBACKFUNC(pAppStMan)(BYTE),             
                     VOID_CALLBACKFUNC(pSetPowerDownTimeout)(BYTE));


/** 
 * @brief KeyScan
 * Scan input pins for change and convert it to events. Function should be
 * placed in ApplicationPoll() routine.
 */
void KeyScan();


#endif /* _KEYMAN_H_ */


