/**
 * @file
 * Handler for Command Class Association Group Info.
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMAND_CLASS_ASSOCIATION_GROUP_INFO_H_
#define _COMMAND_CLASS_ASSOCIATION_GROUP_INFO_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_typedefs.h>
#include <ZW_sysdefs.h>
#include <ZW_classcmd.h>
#include <CommandClass.h>
#include <ZW_TransportEndpoint.h>

/**
 * Returns the version of this CC.
 */
#define CommandClassAssociationGroupInfoVersionGet() ASSOCIATION_GRP_INFO_VERSION

/**
 * @brief Read AGI group name
 * @param[out] pGroupName OUT point to group name
 * @param[in] groupId IN group identifier
 * @param[in] endpoint IN end-point number
 */
extern uint8_t
GetApplGroupName(
    char * pGroupName,
    uint8_t groupId,
    uint8_t endpoint);

/**
 * @brief Get application specific Group Info
 * @param[in] groupId group identifier
 * @param[in] endpoint is the endpoint number
 * @param[out] report pointer to data of type VG_ASSOCIATION_GROUP_INFO_REPORT_VG
 */
extern void
GetApplGroupInfo(
  uint8_t groupId,
  uint8_t endpoint,
  VG_ASSOCIATION_GROUP_INFO_REPORT_VG* report);

/**
 * @brief Returns the number of association groups for a given endpoint.
 * @param[in] endpoint A given endpoint where 0 is the root device.
 * @return Number of association groups.
 */
extern uint8_t GetApplAssoGroupsSize(uint8_t endpoint);

/**
 * @brief Set Application specific Group Command List
 * @param[out] pGroupList pointer to the list
 * @param[in] groupId group identifier
 * @param[in] endpoint is the endpoint number
 */
extern void GetApplGroupCommandList(
    uint8_t * pGroupList,
    uint8_t groupId,
    uint8_t endpoint);

/**
 * @brief Application specific Group Command List Size
 * @param[in] groupId group identifier
 * @param[in] endpoint is the endpoint number
 * @return size
 */
extern uint8_t GetApplGroupCommandListSize(
    uint8_t groupId,
    uint8_t endpoint);

/**
 * @brief Handler for Association Group Info Command Class.
 * @param[in] rxOpt receive options of type RECEIVE_OPTIONS_TYPE_EX
 * @param[in] pCmd Payload from the received frame
 * @param[in] cmdLength Number of command bytes including the command
 * @return receive frame status.
 */
received_frame_status_t handleCommandClassAssociationGroupInfo(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength);

#endif
