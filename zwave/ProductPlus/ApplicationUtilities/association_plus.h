/**
 *
 * Copyright (c) 2001-2013
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 * @file association_plus.h
 *
 * @brief Header file for associaction.c. This module implements
 *        functions that makes it easy to add association commandclass
 *        support to applications.
 *        All far variables (NVM offsets) should be defined in the application's eeprom.h module
 *        in the struct t_nvmApplDescriptor
 *
 * Author: Thomas Roll
 *
 * Last Changed By: $Author: tro $
 * Revision: $Revision: 0.00 $
 * Last Changed: $Date: 2013/06/21 10:18:01 $
 *
 */

#ifndef _ASSOCIATION_PLUS_H_
#define _ASSOCIATION_PLUS_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_typedefs.h>
#include "config_app.h"

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
/*Support for more groups can be enabled*/

#define ASSOCIATION_SIZE            (MAX_ASSOCIATION_GROUPS * MAX_ASSOCIATION_IN_GROUP)

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/*============================ handleAssociationGetnodeList =================
** Function description
** Deliver group number node list.
** Return status on job: TRUE/FALSE
** Side effects:
**
**-------------------------------------------------------------------------*/
BOOL
handleAssociationGetnodeList(
  BYTE groupIden,             /* IN  Group identifier*/
  BYTE** ppNodeId,          /* OUT Doublepointer to list of nodes in group*/
  BYTE* pNbrOfNodes);        /* OUT Number of nodes in group*/


/*============================ AssociationGetNode ===============================
** Function description
** Get Node Id from a Association group.
**
** Side effects:
**
**-------------------------------------------------------------------------*/


/**
 * @brief AssociationGetNode
 * Comment function...
 * @param groupIden  Grouping Identifier
 * @param groupIndex Group index
 * @return node Id. Return 0 if not legal
 */
BYTE AssociationGetNode(
  BYTE groupIden,
  BYTE groupIndex);

/*============================   AssociationStoreAll   ======================
**    Function description
**      Should be implmented by application to Store all group[]
**      in non-volatile memory
**    Side effects:
**
**--------------------------------------------------------------------------*/
void AssociationStoreAll( void );
/*============================   AssociationClearAll   ======================
**    Function description
**      Should be implmented by application to Clear all of group[]
**      in non-volatile memory
**    Side effects:
**
**--------------------------------------------------------------------------*/
void AssociationClearAll( void );

/*============================   AssociationInit   ======================
**    Function description
**      Should be implmented by application to read all groups from
**      non-volatile memory to group[]
**    Side effects:
**
**--------------------------------------------------------------------------*/
void AssociationInit(BOOL forceClearMem); /*IN reset groups list*/


/*============================   AssociationAdd   ======================
**    Function description
**      Adds the nodes in nodeIdP to the group
**    Side effects:
**
**--------------------------------------------------------------------------*/
extern void
AssociationAdd(
  BYTE groupIden,     /*IN group identifier to add nodes to*/
  BYTE_P nodeIdP, /*IN pointer to list of nodes*/
  BYTE noOfNodes); /*IN number of nodes in List*/

/*============================   AssociationRemove   ======================
**    Function description
**      Removes association members from one or all group
**      If group = 0 nodeIds in nodeIdP list is removed from all groups
**      If noOfNodes = 0 group is removed
**      if both is 0 all groups and nodes is removed
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
AssociationRemove(
  BYTE groupIden,      /*IN group identifier to remove nodes from*/
  BYTE_P nodeIdP,  /*IN pointer to array of nodes to remove*/
  BYTE noOfNodes);  /*IN number of nodes in list*/


/*=======================   AssociationGetLifeLineNodeID   ========================
**    Function description
**      Reads the nodeID asscoiated to the lifeLine group
**
**    Side effects:
**
**------------------------------------------------------------------------------*/
BYTE /*RET the nodeID of the lifeline node, 0xFF if not associated*/
AssociationGetLifeLineNodeID(void);


/*============================ handleGetMaxAssociationGroups ================
** Function description
** Return max association groups..
**
** Side effects:
**
**-------------------------------------------------------------------------*/
BYTE
handleGetMaxAssociationGroups(void);


/*========================   GetApplAssoGroupsSize  ===========
**   get number of the application association groups
**   return none
**
**   Side effects: none
**--------------------------------------------------------------------------*/
BYTE GetApplAssoGroupsSize(void);

#endif /* _ASSOCIATION_PLUS_H_ */

