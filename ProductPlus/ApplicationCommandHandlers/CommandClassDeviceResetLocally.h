/**
 * @file
 * Handler for Command Class Device Reset Locally.
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMAND_CLASS_DEVICE_RESET_LOCALLY_H_
#define _COMMAND_CLASS_DEVICE_RESET_LOCALLY_H_

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
#define CommandClassDeviceResetLocallyVersionGet() DEVICE_RESET_LOCALLY_VERSION

/** 
 * @brief handleCommandClassDeviceResetLocally
 * Handler for Device Reset Locally CC
 * @param completedFunc Callback function pointer. Use the callback call to reset
 * the node. This function callback must be implemented!
 */
void 
handleCommandClassDeviceResetLocally( VOID_CALLBACKFUNC(completedFunc)(BYTE));

#endif
