/**
 * @file
 * Handler for Command Class Multi Channel.
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _CMULTICHAN_H_
#define _CMULTICHAN_H_

#include "config_app.h"
#include <ZW_basis_api.h>
#include <CommandClass.h>
#include <ZW_TransportEndpoint.h>


/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * Returns the version of this CC.
 */
#define CmdClassMultiChannelVersionGet() MULTI_CHANNEL_VERSION_V4
#define CmdClassMultiChannelGet() CmdClassMultiChannelVersionGet()

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/**
 * @brief Handler for multi channel commands.
 * @param[in] rxOpt Frame header info
 * @param[in] pCmd Payload from the received frame
 * @param[in] cmdLength Number of command bytes including the command
 */
received_frame_status_t
MultiChanCommandHandler(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength);

/**
 * @brief Encapsulates a frame with Multi Channel.
 * @param[in,out] ppData Pointer to data.
 * @param[in,out] dataLength Pointer to data length.
 * @param[in] pTxOptionsEx Pointer to transmit options.
 */
void
CmdClassMultiChannelEncapsulate(
  BYTE **ppData,
  BYTE *dataLength,
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx);


#endif /* _CVERSION_H_ */
