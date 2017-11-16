/**
 * @file
 * Handler for Command Class Powerlevel.
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMAND_CLASS_POWERLEVEL_H_
#define _COMMAND_CLASS_POWERLEVEL_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_sysdefs.h>
#include <ZW_pindefs.h>
#include <ZW_evaldefs.h>
#include <ZW_classcmd.h>

/**
 * Returns the version of this CC.
 */
#define CommandClassPowerLevelVersionGet() POWERLEVEL_VERSION

/* Power level definitions */
#define ZW_TEST_NOT_A_NODEID                            0x00

/*==============================   loadStatusPowerLevel  ============
**
**  Function:  loads power level status from nvram
**
**  Side effects: None
**
**--------------------------------------------------------------------------*/
void
loadStatusPowerLevel(VOID_CALLBACKFUNC(pStopPowerDownTimer)(void),VOID_CALLBACKFUNC(pStartPowerDownTimer)(void));

/*==============================   loadInitStatusPowerLevel  ============
**
**  Function:  loads initial power level status from nvram
**
**  Side effects: None
**
**--------------------------------------------------------------------------*/
void
loadInitStatusPowerLevel(VOID_CALLBACKFUNC(pStopPowerDownTimer)(void),VOID_CALLBACKFUNC(pStartPowerDownTimer)(void));

/**
 * Handler for the Command Class Powerlevel.
 * @param option Receive options.
 * @param sourceNode Source node ID.
 * @param pCmd Pointer to command.
 * @param cmdLength Length of command.
 */
void handleCommandClassPowerLevel(
    BYTE option,
    BYTE sourceNode,
    ZW_APPLICATION_TX_BUFFER * pCmd,
    BYTE cmdLength);

/**
 * @brief Returns whether a powerlevel test is in progress.
 * @return TRUE if in progress, FALSE otherwise.
 */
BOOL
CommandClassPowerLevelIsInProgress(void);

#endif
