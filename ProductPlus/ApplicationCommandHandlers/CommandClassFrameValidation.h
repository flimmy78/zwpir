/**
 * @file
 * Handler for Command Class Frame Validation.
 *
 * isCmdClassFrameInvalid is called with aValidCommandTable which must be pointing
 * to a table containing valid Commands and their minimum size pairs -
 * Here below an example for Command Class BASIC
 * VALID_COMMANDCLASS_FRAME CODE aVALID_COMMAND_CLASS_BASIC_FRAME_TABLE[] = {{BASIC_SET, sizeof(ZW_BASIC_SET_FRAME)},
 *                                                                          {BASIC_GET, sizeof(ZW_BASIC_GET_FRAME)}};
 * Usage:
 * CmdClassFrameValidate(bCmd, bCmdLength, aVALID_COMMAND_CLASS_BASIC_FRAME_TABLE,
 *                       sizeof(aVALID_COMMAND_CLASS_BASIC_FRAME_TABLE))
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMAND_CLASS_FRAME_VALIDATION_H_
#define _COMMAND_CLASS_FRAME_VALIDATION_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_sysdefs.h>
#include <ZW_classcmd.h>


/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

typedef struct _VALID_COMMANDCLASS_FRAME_
{
  BYTE cmd;
  BYTE cmdLength_Min;
 } VALID_COMMANDCLASS_FRAME;


/* Definitions of CmdClassFrameValidate Return values */
/* Command Class Command size VALID according to delivered requirements */
#define CMDCLASS_COMMAND_SIZE_VALID       0x00
/* Command Class Command size NOT VALID according to delivered requirements */
#define CMDCLASS_COMMAND_SIZE_NOT_VALID   0x01
/* Command Class Command size requirements not found */
#define CMDCLASS_COMMAND_NOT_FOUND        0xFF


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/* isCmdClassFrameInvalid is called with aValidCommandTable which must be pointing */
/* to a table containing valid Commands and their minimum size pairs -      */
/* Here below an example for Command Class BASIC                            */
/* VALID_COMMANDCLASS_FRAME CODE aVALID_COMMAND_CLASS_BASIC_FRAME_TABLE[] = {{BASIC_SET, sizeof(ZW_BASIC_SET_FRAME)}, */
/*                                                                          {BASIC_GET, sizeof(ZW_BASIC_GET_FRAME)}}; */
/* Usage: */
/* CmdClassFrameValidate(bCmd, bCmdLength, aVALID_COMMAND_CLASS_BASIC_FRAME_TABLE, */
/*                       sizeof(aVALID_COMMAND_CLASS_BASIC_FRAME_TABLE))    */
/*                                                                          */


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
  BYTE bCmd,              /* IN CommandClass Command to check if valid according to minimum payload size requirements */
  BYTE bCmdLength,         /* IN CommandClass Command received Payload size */
  VALID_COMMANDCLASS_FRAME *aValidCommandTable, /* IN Pointer to first member of Command Class Command validation table */
  BYTE bTableSize);       /* IN Size (in bytes - sizeof) of specified Command Class Command validation table */


#endif  /* #ifndef _COMMAND_CLASS_FRAME_VALIDATION_H_ */
