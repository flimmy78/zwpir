/**
 * @file
 * Handler for Command Class Z-Wave Plus Info.
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMAND_CLASS_ZWAVE_PLUS_INFO_H_
#define _COMMAND_CLASS_ZWAVE_PLUS_INFO_H_

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
#define CommandClassZWavePlusVersion() ZWAVEPLUS_INFO_VERSION_V2
 
/** 
 * @brief handleCommandClassZWavePlusInfo
 * handler for ZWave Plus Info CC
 * @param option IN Frame header info
 * @param sourceNode IN Command sender Node ID
 * @param pCmd  Payload pointer from the received frame, the union should 
 * be used to access the fields.
 * @param cmdLength IN Number of command bytes including the command
 * @return description..
 */
void
handleCommandClassZWavePlusInfo(
  BYTE  option,                    
  BYTE  sourceNode,                
  ZW_APPLICATION_TX_BUFFER *pCmd,  
  BYTE   cmdLength                
);

#endif
