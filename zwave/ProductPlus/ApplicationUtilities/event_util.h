/**
 *
 * Copyright (c) 2001-2013
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 * @file util_event.h
 *
 * @brief Header file for event_util.c. This module implements
 *        a function that initialize the function pointer of the Commands
 *        events handler. The Event handler notify the application about
 *        The commands activity
 *
 *
 *
 *
 * Author: Samer Seoud
 *
 * Last Changed By: $Author: tro $
 * Revision: $Revision: 0.00 $
 * Last Changed: $Date: 2013/06/21 10:18:01 $
 *
 */

#ifndef _EVENT_UTIL_H_
#define _EVENT_UTIL_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/


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
 * @brief InitCmdReceivedEvent
 * Initaliase the Command received handler function pointer
 * @param void (code * CmdReceivedEvent)(BYTE, BYTE)
 *   The fucntion pointer of the command received event handler
 * @return none.
 */
extern void
InitCmdReceivedEvent(
  VOID_CALLBACKFUNC(CmdReceivedEvent)(BYTE, BYTE));

/**
 * @brief CmdReceivedEvent
 *  Notify application about the received command
 *
 * @param commandClass The Command class ID of the received command
 * @param command  The received Command ID
 *
 * @return none.
 */
extern void
CmdReceivedEvent(BYTE commandClass, BYTE command);

#endif /* _EVENT_UTIL_H_ */

