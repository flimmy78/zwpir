/***************************************************************************
*
* Copyright (c) 2001-2013
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Implements an event interface to notify the application
*              Commands activity
*
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2013/06/21 10:15:17 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <ZW_uart_api.h>
#include <event_util.h>


/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/



/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
/* this function pointer for the application defined function used to notify the application that a command is received */
static VOID_CALLBACKFUNC(ZCB_CmdReceivedEvent)(BYTE cmdClass,   /* The command class of the command event */
                                               BYTE cmd);       /* The command that the event belong to */


/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/



/*============================ InitCmdReceivedEvent =================
** Function description
** Initailase the Command received handler function pointer
** Return
** Side effects:
**
**-------------------------------------------------------------------------*/

void
InitCmdReceivedEvent(
  VOID_CALLBACKFUNC(CmdReceivedEvent)(BYTE, BYTE))
{
  ZCB_CmdReceivedEvent = CmdReceivedEvent;
}


void
CmdReceivedEvent(BYTE commandClass, BYTE command)
{
  if (NULL != ZCB_CmdReceivedEvent)
  {
    ZCB_CmdReceivedEvent(commandClass, command);
  }
}

