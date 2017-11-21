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
#include <ZW_pindefs.h>
#include <ZW_evaldefs.h>
#include <ZW_classcmd.h>

/**
 * Returns the version of this CC.
 */
#define CommandClassAssociationGroupInfoVersionGet() ASSOCIATION_GRP_INFO_VERSION

/*=============================   GetApplGroupName  ==================================
**
** 
**  Side effects: None
**
**-----------------------------------------------------------------------------------*/
extern void 
GetApplGroupName(
  BYTE* n, 
  BYTE groupId
);

/*========================   GetApplGroupNameLength  ==================================
**
** 
**  Side effects: None
**
**-----------------------------------------------------------------------------------*/
extern BYTE 
GetApplGroupNameLength(
  BYTE groupId
);

/*=========================   GetApplGroupInfo  ================================
**
** 
**  Side effects: None
**
**-----------------------------------------------------------------------------------*/
extern void 
GetApplGroupInfo(
  BYTE groupId,
  VG_ASSOCIATION_GROUP_INFO_REPORT_VG* report
);

/*========================   GetApplAssoGroupsSize  ===========
**   get number of the application association groups
**   return none
**
**   Side effects: none
**--------------------------------------------------------------------------*/
extern BYTE
GetApplAssoGroupsSize( void );
/*================  ==========   setApplGroupCommandList  ============================
**
** 
**  Side effects: None
**
**-----------------------------------------------------------------------------------*/
extern void 
setApplGroupCommandList(
  BYTE* l, 
  BYTE groupId
);

/*===========================   getApplGroupCommandListSize  ==========================
**
** 
**  Side effects: None
**
**-----------------------------------------------------------------------------------*/
extern BYTE 
getApplGroupCommandListSize(
  BYTE groupId
);

/**
 * @brief Handler for Association Group Info CC
 * @param option Frame header info
 * @param sourceNode Command sender Node ID
 * @param pCmd Payload from the received frame, the union should be used to access the fields.
 * @param cmdLength Number of command bytes including the command
 */
void
handleCommandClassAssociationGroupInfo(
    BYTE option,
    BYTE sourceNode,
    ZW_APPLICATION_TX_BUFFER *pCmd,
    BYTE cmdLength);

#endif
