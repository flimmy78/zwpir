/***************************************************************************
*
* Copyright (c) 2001-2013
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Command Class Command Frame validation
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
#include <ZW_classcmd.h>
#include <CommandClassFrameValidation.h>

#include "config_app.h"

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/


/*=============================   CmdClassFrameValidate  ===========================
**
**  Function:  Validation of Application Frame according to minimum size requirement
**    Returns:  ZERO (CMDCLASS_COMMAND_SIZE_VALID) - CmdClass-Command Frame is Valid
**              CMDCLASS_COMMAND_SIZE_NOT_VALID - CmdClass-Command is out of bounds
**              CMDCLASS_COMMAND_NOT_FOUND - CmdClass-Command not found
**
**  Side effects: None
**
**--------------------------------------------------------------------------------*/
BYTE
CmdClassFrameValidate(
  BYTE bCmd,        /* IN CommandClass Command to check if valid according to minimum payload size requirements */
  BYTE bCmdLength,  /* IN CommandClass Command received Payload size */
  VALID_COMMANDCLASS_FRAME *aValidCommandTable, /* IN Pointer to first member of Command Class Command validation table */
  BYTE bTableSize)  /* IN Size (in bytes - sizeof) of specified Command Class Command validation table */
{
  BYTE retVal = CMDCLASS_COMMAND_NOT_FOUND;
  BYTE i;

  /* Determine number of members in specified VALID_COMMANDCLASS_FRAME table */
  bTableSize /= sizeof(VALID_COMMANDCLASS_FRAME);
  for (i = 0; i < bTableSize; i++)
  {
    if (bCmd == aValidCommandTable[i].cmd)
    {
      /* Found Command */
      if (aValidCommandTable[i].cmdLength_Min <= bCmdLength)
      {
        /* Command size OK according to minimum requirements */
        retVal = CMDCLASS_COMMAND_SIZE_VALID;
      }
      else
      {
        /* Command size NOT Valid according to minimum requirements */
        retVal = CMDCLASS_COMMAND_SIZE_NOT_VALID;
      }
      break;
    }
    /* Command not found - try next */
  }
  return retVal;
}
