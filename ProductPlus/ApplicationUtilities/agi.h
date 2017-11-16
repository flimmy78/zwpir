/**
 *
 * Copyright (c) 2001-2014
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 * @file agi.h
 *
 * @brief AGI module. AGI module is interfacing association_plus module for reading 
 * association group node list. Current version do not support endpoints!
 * 
 * Example of using double pointer:
 * Current module use double pointer for delevering pointer to the modules data-area. 
 * Example calling GetNodeIdListFirst(...):
 *
 * CMD_CLASS_GRP cmdGrp;
 * NODE_LIST nList = NULL;
 * BYTE nodeListLen = 0;
 *
 *  AGI_NodeIdListInit(&nList);
 *
 *  // Seach through AGI table for AGI-profile ASSOCIATION_GROUP_INFO_REPORT_PROFILE_SENSOR and deliver 
 *  // node list for each group.
 *  while(TRUE == AGI_ResourceNodeIdListLookup( ASSOCIATION_GROUP_INFO_REPORT_PROFILE_SENSOR, 
 *                                 0, &cmdGrp, &pNodeList, &nodeListLen))
 *  {
 *    BYTE i = 0;
 *    // Now we have a pointer (pNodeList) to the node list and the length 
 *    // (nodeListLen)of the node list for  AGI profile ASSOCIATION_GROUP_INFO_REPORT_PROFILE_SENSOR
 *    for(i = 0; i < nodeListLen; i++)
 *    {
 *      // Print out the node list.
 *      ZW_DEBUG_SEND_NUM(pNodeList[i]);
 *    }
 *  }
 *
 * Author: Thomas Roll
 *
 * Last Changed By: $Author: tro $
 * Revision: $Revision: 0.00 $
 * Last Changed: $Date: 2014/05/28 11:02:25 $
 *
 */

#ifndef _AGI_H_
#define _AGI_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/


/**
 * Command class-group include on command class and one command. 
 */
typedef struct _CMD_CLASS_GRP_
{
  BYTE cmdClass; /**< Command class*/
  BYTE cmd;      /**< Command*/
} CMD_CLASS_GRP;

typedef struct _NODE_LIST_
{
  BYTE* pNodeList; 
  BYTE len;
  BYTE* pCurrentNode; 
  CMD_CLASS_GRP* pCurrentCmdGrp;
} NODE_LIST;


typedef struct AGI_PROFILE
{
  BYTE profile_MS; /**< AGI profile of type: ASSOCIATION_GROUP_INFO_REPORT_PROFILE_...*/
  BYTE profile_LS; /**< AGI profile of type: ASSOCIATION_GROUP_INFO_REPORT_PROFILE_...*/
}  AGI_PROFILE;

typedef struct _AGI_GROUP_
{
  BYTE profile_MS; /**< AGI profile of type: ASSOCIATION_GROUP_INFO_REPORT_PROFILE_...*/
  BYTE profile_LS; /**< AGI profile of type: ASSOCIATION_GROUP_INFO_REPORT_PROFILE_...*/
  CMD_CLASS_GRP cmdGrp; /**< AGI Profile cmd class group*/
  BYTE groupName[42]; /**< AGI Profile group-name UTF-8 format*/
} AGI_GROUP;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/


 /** 
 * @brief AGI_LifeLineGroupSetup
 * Setup AGI Lifeline command classes and commands. Create a CMD_CLASS_GRP 
 * array.
 * @param pCmdGrpList is list of lifeline command groups (cmd classes and commands).
 * @param listSize is number of command groups in lifeline list (array size).
 */
void AGI_LifeLineGroupSetup( CMD_CLASS_GRP* pCmdGrpList, BYTE listSize);

 /** 
 * @brief AGI_ResourceGroupSetup
 * Setup AGI list of tables for one endpoint. 
 * @param pTable is AGI table for one endpoint.
 * @param tableSize is number of groups in table.
 * @param endpoint device endpoint number for the AGI table. Enpoint 0 and 1 will be 
 * handle as the same number!
 */
void AGI_ResourceGroupSetup(AGI_GROUP* pTable, BYTE tableSize, BYTE endpoint);



/** 
 * @brief SetSpecificGroupId
 * Function description
 * Set specific group Id. THis function is intended to assist the association 
 * of multi-button wall controller devices so that individual buttons can be 
 * mapped to different target devices. Please read "Z-Wave command Class 
 * specification" chapter "Association Specific Group Get Command".
 *
 * @param profile is AGI-profile.
 * @param endpoint is source node endpoint.
 * @return TRUE if it found the profile else FALSE.
 */
BOOL AGI_SpecificGroupIdSet( AGI_PROFILE profile, BYTE endpoint);


/** 
 * @brief AGI_LifeLineNodeIdListLookup
 * Get Lifeline node list. 
 * @param ppCmdGrp is a double-pointer to association command class groups.
 * @param pListSize is a pointer deliver number of command class groups.
 * @param ppNodeList is a double pointer to the association module nodelist. 
 * @param pNodeListLen is poiner to length of the nodelist (number of nodes in the list).
 * @return status TRUE if the profile was found else FALSE.
 */
BOOL AGI_LifeLineNodeIdListLookup( CMD_CLASS_GRP** ppCmdGrp,BYTE* pListSize, BYTE** ppNodeList, BYTE* pNodeListLen);


/** 
 * @brief AGI_NodeIdListInit
 * Reset the node list search. Most be called before starting AGI_ResourceNodeIdListLookup(..)
 */
void AGI_NodeIdListInit(NODE_LIST* pNextNode, AGI_PROFILE* pInAgiProfile);


/** 
 * @brief AGI_ResourceNodeIdListLookup
 * Get next node list out from AGI profile. Application should call AGI_ResourceNodeIdListLookup()
 * until it return FALSE. This function do not support lifeline.
 * @param profile is AGI profile
 * @param endpoint is source endpoint.
 * @param ppNodeList is a double pointer to the association module nodelist.
 * @param pNodeListLen is poiner to length of the nodelist (number of nodes in the list).
 * @return status TRUE if the profile was found else FALSE.
 */
BOOL AGI_ResourceNodeIdListLookup( AGI_PROFILE profile,  BYTE endpoint, CMD_CLASS_GRP** ppCmdGrp,  BYTE** ppNodeList, BYTE* pNodeListLen);


/** 
 * @brief AGI_NodeIdListGetNext
 * Return destination nodes dependent of AGI profile. Only one destiantion node is returned per call. 
 * User should call the function until AGI_NodeIdListGetNext return FALSE. AGI_NodeIdListGetNext
 * seach first through AGI lifeline group and the rest.
 * @param pNextNode is destination node of type NODE_LIST.
 * @param pInAgiProfile is a pointer to AGI profile
 * @return boolean, TRUE if not finish else FALSE.
 */
BOOL AGI_NodeIdListGetNext( NODE_LIST* pNextNode);

#endif /* _AGI_H_ */


