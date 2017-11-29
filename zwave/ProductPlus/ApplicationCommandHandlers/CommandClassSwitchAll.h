/**
 * @file
 * Handler for Command Class Switch All.
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMAND_CLASS_ALL_SWITCH_H_
#define _COMMAND_CLASS_ALL_SWITCH_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_classcmd.h>
#include <CommandClass.h>
#include <ZW_TransportEndpoint.h>

/**
 * Returns the version of this CC.
 */
#define CommandClassSwitchAllVersionGet() SWITCH_ALL_VERSION

/**
 * Defines On/Off values for the Switch All command class.
 */
typedef enum {
  CMD_CLASS_SWITCHALL_OFF = 0x00,
  CMD_CLASS_SWITCHALL_ON  = 0xFF
} CMD_CLASS_SWITCHALL_SET;


/**
 * @brief Incoming command class call Set the switch to zero or ON value in application endpoint
 * @param[in] val of type CMD_CLASS_SWITCHALL_SET
 * @param[in] endpoint is the destination endpoint
 */
extern void handleSwitchAll(CMD_CLASS_SWITCHALL_SET val, BYTE endpoint);


/**
 * @brief Handler for command class all switch
 * @param[in] rxOpt receive options of type RECEIVE_OPTIONS_TYPE_EX
 * @param[in] pCmd Payload from the received frame
 * @param[in] cmdLength number of command bytes including the command
 * @return receive frame status.
 */
received_frame_status_t handleCommandClassSwitchAll(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength);

#endif
