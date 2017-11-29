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
#include <CommandClass.h>
#include <ZW_TransportEndpoint.h>

/**
 * Returns the version of this CC.
 */
#define CommandClassPowerLevelVersionGet() POWERLEVEL_VERSION

/**
 * @brief loadStatusPowerLevel
 * Load parameters from NVM
 * @param pStopPowerDownTimer is a function pointer to stop power down timer
 * @param pStartPowerDownTimer is a function pointer to start power down timer
 */
void loadStatusPowerLevel(VOID_CALLBACKFUNC(pStopPowerDownTimer)(void),VOID_CALLBACKFUNC(pStartPowerDownTimer)(void));


 /**
 * @brief loadInitStatusPowerLevel
 * loads initial power level status from nvram
 * @param pStopPowerDownTimer is a function pointer to stop power down timer
 * @param pStartPowerDownTimer is a function pointer to start power down timer
 */
void loadInitStatusPowerLevel(VOID_CALLBACKFUNC(pStopPowerDownTimer)(void),VOID_CALLBACKFUNC(pStartPowerDownTimer)(void));


/**
 * @brief handleCommandClassPowerLevel
 * his function called when the node receives a power level command
 * @param rxOpt IN receive options of type RECEIVE_OPTIONS_TYPE_EX
 * @param pCmd IN Payload from the received frame, the union should be used to access
 * the fields.
 * @param cmdLength IN Number of command bytes including the command.
 * @return receive frame status.
 */
received_frame_status_t handleCommandClassPowerLevel(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE   cmdLength);

/**
 * @brief Returns whether a powerlevel test is in progress.
 * @return TRUE if in progress, FALSE otherwise.
 */
BOOL
CommandClassPowerLevelIsInProgress(void);

#endif
