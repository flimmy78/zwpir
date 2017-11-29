/**
 * @file
 * Helper module for Command Class Association Group Information.
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include "config_app.h"
#include <ZW_mem_api.h>
#include <ZW_string.h>
#include <association_plus.h>
#include <agi.h>
#include <misc.h>
#include <ZW_uart_api.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_AGI
#define ZW_DEBUG_AGI_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_AGI_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_AGI_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_AGI_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_AGI_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_AGI_SEND_BYTE(data)
#define ZW_DEBUG_AGI_SEND_STR(STR)
#define ZW_DEBUG_AGI_SEND_NUM(data)
#define ZW_DEBUG_AGI_SEND_WORD_NUM(data)
#define ZW_DEBUG_AGI_SEND_NL()
#endif

#define AGI_STRING_LEN 42

typedef struct _AGI_LIFELINE_
{
  char grpName[AGI_STRING_LEN];
  CMD_CLASS_GRP* pCmdGrpList;
  uint8_t listSize;
} AGI_LIFELINE;

typedef struct _AGI_TABLE_EP_
{
  AGI_GROUP* pTable;
  uint8_t tableSize;
} AGI_TABLE_EP;

typedef struct _AGI_TABLE_
{
  AGI_LIFELINE lifeLineEndpoint[NUMBER_OF_ENDPOINTS + 1];
  AGI_TABLE_EP tableEndpoint[NUMBER_OF_ENDPOINTS + 1];
} AGI_TABLE;

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

static AGI_TABLE myAgi;
static uint8_t m_lastActiveGroupId = 1;
static NODE_LIST nodeList;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

// Nothing here.
/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
static TRANSMIT_OPTIONS_TYPE_EX* ReqNodeListTxOptions(NODE_LIST* pDestNodeList,
                                                      uint8_t sourceEndpoint);

void
AGI_Init(void)
{
  uint8_t i = 0;
  const char lifelineText[] = "Lifeline";

  m_lastActiveGroupId = 1;

  memset((uint8_t * )&myAgi, 0x00, sizeof(myAgi));
  for (i = 0; i < (NUMBER_OF_ENDPOINTS + 1); i++)
  {
    memcpy(
           myAgi.lifeLineEndpoint[i].grpName,
           (uint8_t * )lifelineText,
           ZW_strlen((uint8_t * )lifelineText));
  }
}

void
AGI_LifeLineGroupSetup(
                       CMD_CLASS_GRP* pCmdGrpList,
                       uint8_t listSize,
                       const char * pGrpName,
                       uint8_t endpoint)
{
  uint8_t stringLen;

  if (NUMBER_OF_ENDPOINTS < endpoint)
  {
    return; // Invalid endpoint => return.
  }

  myAgi.lifeLineEndpoint[endpoint].pCmdGrpList = pCmdGrpList;
  myAgi.lifeLineEndpoint[endpoint].listSize = listSize;

  if (IS_NULL(pGrpName))
  {
    return;
  }

  stringLen = ZW_strlen((uint8_t *)pGrpName);

  if (AGI_STRING_LEN < stringLen)
  {
    memcpy(myAgi.lifeLineEndpoint[endpoint].grpName, (uint8_t * )pGrpName, AGI_STRING_LEN - 1);
    myAgi.lifeLineEndpoint[endpoint].grpName[AGI_STRING_LEN - 1] = '\0'; /* secure null character */
  }
  else
  {
    memcpy(myAgi.lifeLineEndpoint[endpoint].grpName, (uint8_t * ) pGrpName, stringLen);
  }
}

void
AGI_ResourceGroupSetup(AGI_GROUP pTable[], uint8_t tableSize, uint8_t endpoint)
{
  if (IS_NULL(pTable))
  {
    tableSize = 0;
  }

  if (NUMBER_OF_ENDPOINTS >= endpoint)
  {
    myAgi.tableEndpoint[endpoint].tableSize = tableSize;
    myAgi.tableEndpoint[endpoint].pTable = (AGI_GROUP*)pTable;
  }
}

uint8_t
GetApplGroupName(
                 char * pGroupName,
                 uint8_t groupId,
                 uint8_t endpoint)
{
  uint8_t nameLength = 0;
  char * pSourceGroupName;
  const char errorText[] = "Invalid group";

  if ((IS_NULL(pGroupName)) || (NUMBER_OF_ENDPOINTS < endpoint))
  {
    return nameLength;
  }

  // tableSize does not include Lifeline. Hence, +1.
  if ((groupId > (myAgi.tableEndpoint[endpoint].tableSize + 1)) || (0 == groupId))
  {
    pSourceGroupName = (char *)errorText;
  }
  else
  {
    if (1 == groupId)
    {
      pSourceGroupName = myAgi.lifeLineEndpoint[endpoint].grpName;
    }
    else
    {
      /*
       * myAgi.tableEndpoint[endpoint].pTable represents all groups not being lifeline groups.
       * Hence, the lowest possible group is 2 since lifeline is group 1.
       * the first index in array = given group - 2.
       */
      pSourceGroupName = myAgi.tableEndpoint[endpoint].pTable[groupId - 2].groupName;
    }
  }
  nameLength = ZW_strlen((uint8_t *)pSourceGroupName);
  memcpy(
         (uint8_t * )pGroupName,
         (uint8_t * )pSourceGroupName,
         nameLength);

  return nameLength;
}

uint8_t GetApplAssoGroupsSize(uint8_t endpoint)
{
  if (NUMBER_OF_ENDPOINTS < endpoint)
  {
    return 0; /** Error!!*/
  }
  return 1 + myAgi.tableEndpoint[endpoint].tableSize; /* Lifeline group + grouptable size.*/
}

/**
 * CommandClassGroupInfo.h
 */
void
GetApplGroupInfo(
                 uint8_t groupId,
                 uint8_t endpoint,
                 VG_ASSOCIATION_GROUP_INFO_REPORT_VG* report)
{
  if (NUMBER_OF_ENDPOINTS < endpoint)
  {
    return; /** Error!!*/
  }

  if (groupId >= myAgi.tableEndpoint[endpoint].tableSize + 2)
  {
    /*Not legal groupId!*/
    report->groupingIdentifier = 0;
    report->mode = 0; /**/
    report->profile1 = 0; /* MSB */
    report->profile2 = 0; /* LSB */
    report->reserved = 0; /**/
    report->eventCode1 = 0; /* MSB */
    report->eventCode2 = 0; /* LSB */
  }

  if (1 == groupId)
  {
    /*Report all association groups in one message!*/
    report->groupingIdentifier = groupId;
    report->mode = 0; /**/
    report->profile1 = ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL; /* MSB */
    report->profile2 = ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_LIFELINE; /* LSB */
    report->reserved = 0; /**/
    report->eventCode1 = 0; /* MSB */
    report->eventCode2 = 0; /* LSB */
  }
  else
  {
    report->groupingIdentifier = groupId;
    report->mode = 0; /**/
    report->profile1 = myAgi.tableEndpoint[endpoint].pTable[groupId - 2].profile.profile_MS;
    report->profile2 = myAgi.tableEndpoint[endpoint].pTable[groupId - 2].profile.profile_LS;
    report->reserved = 0; /**/
    report->eventCode1 = 0; /* MSB */
    report->eventCode2 = 0; /* LSB */
  }

}

void GetApplGroupCommandList(
                             uint8_t * pGroupList,
                             uint8_t groupId,
                             uint8_t endpoint)
{
  if (NUMBER_OF_ENDPOINTS < endpoint)
  {
    return; /** Error!!*/
  }

  if (groupId >= myAgi.tableEndpoint[endpoint].tableSize + 2)
  {
    /*Not legal groupId!*/
    *pGroupList = 0x00;
    return;
  }

  if (groupId == 1)
  {
    memcpy(pGroupList, (BYTE_P )myAgi.lifeLineEndpoint[endpoint].pCmdGrpList,
           myAgi.lifeLineEndpoint[endpoint].listSize * sizeof(CMD_CLASS_GRP));
  }
  else
  {
    memcpy(pGroupList, (BYTE_P )&myAgi.tableEndpoint[endpoint].pTable[groupId - 2].cmdGrp,
           sizeof(CMD_CLASS_GRP));
  }
}

uint8_t GetApplGroupCommandListSize(
                                    uint8_t groupId,
                                    uint8_t endpoint)
{
  uint8_t size = 0;

  if (NUMBER_OF_ENDPOINTS < endpoint)
  {
    return 0; /** Error!!*/
  }

  if (groupId >= myAgi.tableEndpoint[endpoint].tableSize + 2)
  {
    /*Not legal groupId!*/
    return 0;
  }

  if (groupId == 1)
  {
    size = myAgi.lifeLineEndpoint[endpoint].listSize * sizeof(CMD_CLASS_GRP);
  }
  else
  {
    size = sizeof(CMD_CLASS_GRP);
  }
  return size;
}

uint8_t
ApplicationGetLastActiveGroupId(void)
{
  return m_lastActiveGroupId;
}

static TRANSMIT_OPTIONS_TYPE_EX*
ReqNodeListTxOptions(NODE_LIST* pDestNodeList, uint8_t sourceEndpoint)
{
  static TRANSMIT_OPTIONS_TYPE_EX reqTxOpt;

  reqTxOpt.sourceEndpoint = sourceEndpoint;

  reqTxOpt.txOptions = ZWAVE_PLUS_TX_OPTIONS;
  reqTxOpt.pList = pDestNodeList->pNodeList;
  reqTxOpt.list_length = pDestNodeList->len;
  reqTxOpt.S2_groupID = 0;

  return &reqTxOpt;
}

/**
 * SearchCmdClass
 */
BOOL
SearchCmdClass(CMD_CLASS_GRP cmdGrp, CMD_CLASS_GRP* pCmdGrpList, uint8_t listSize)
{
  while ((cmdGrp.cmdClass != pCmdGrpList[listSize - 1].cmdClass) && (0 < (listSize - 1)))
  {
    listSize--;
  }
  if (cmdGrp.cmdClass == pCmdGrpList[listSize - 1].cmdClass)
  {
    return TRUE;
  }
  return FALSE;
}

/**
 * SearchCmdClass
 */
uint8_t
SearchProfile(AGI_PROFILE profile, CMD_CLASS_GRP cmdGrp, AGI_TABLE_EP* pAgiTable)
{
  uint8_t grpId = 0;

  for (grpId = 0; grpId < pAgiTable->tableSize; grpId++)
  {
    /*Find profile*/
    if ((profile.profile_MS == pAgiTable->pTable[grpId].profile.profile_MS) &&
        (profile.profile_LS == pAgiTable->pTable[grpId].profile.profile_LS))
    {
      if ( TRUE == SearchCmdClass(cmdGrp, &pAgiTable->pTable[grpId].cmdGrp, 1))
      {
        return grpId;
      }
    }
  }
  return 0xFF;
}

TRANSMIT_OPTIONS_TYPE_EX * ReqNodeList(
                                       AGI_PROFILE* pProfile,
                                       CMD_CLASS_GRP* pCurrentCmdGrp,
                                       uint8_t sourceEndpoint)
{
  static MULTICHAN_NODE_ID nodeListCombined[MAX_ASSOCIATION_IN_GROUP * 2]; /**< handle 2 node lists: lifeline + one list more */
  uint8_t nodeListCombinedLength = 0;

  NODE_LIST_STATUS status;
  uint8_t grpId = 0;
  TRANSMIT_OPTIONS_TYPE_EX* pTxOptionsEx = NULL;
  nodeList.pNodeList = NULL;
  nodeList.len = 0;
  nodeList.sourceEndpoint = sourceEndpoint;

  if (NUMBER_OF_ENDPOINTS < sourceEndpoint)
  {
    return NULL; /** Error!!*/
  }

  if (TRUE == SearchCmdClass(*pCurrentCmdGrp,
                             myAgi.lifeLineEndpoint[sourceEndpoint].pCmdGrpList,
                             myAgi.lifeLineEndpoint[sourceEndpoint].listSize))
  {
    /*endpoint is always 0 for lifeline!!*/
    status = handleAssociationGetnodeList(1, 0, &(nodeList.pNodeList), &(nodeList.len));
    if (status != NODE_LIST_STATUS_SUCCESS)
    {
      nodeList.pNodeList = NULL;
      nodeList.len = 0;
    }
    else
    {
      m_lastActiveGroupId = 1;
      /*Find lifeline nodelist*/
      pTxOptionsEx = ReqNodeListTxOptions(&nodeList, nodeList.sourceEndpoint);
    }
  }

  grpId = SearchProfile(*pProfile, *pCurrentCmdGrp, &myAgi.tableEndpoint[sourceEndpoint]);

  if (0xFF != grpId)
  {
    if (NON_NULL(nodeList.pNodeList))
    {
      nodeListCombinedLength = nodeList.len;
      memcpy((uint8_t* )nodeListCombined, (uint8_t* )nodeList.pNodeList,
             nodeList.len * sizeof(MULTICHAN_NODE_ID));
      nodeList.pNodeList = NULL;
      nodeList.len = 0;
    }

    status = handleAssociationGetnodeList(grpId + 2, sourceEndpoint, &(nodeList.pNodeList),
                                          &(nodeList.len));
    if (status != NODE_LIST_STATUS_SUCCESS)
    {
      if (0 != nodeListCombinedLength)
      {
        /* Don't forget the nodelist lifeline!*/
        nodeList.pNodeList = nodeListCombined;
        nodeList.len = nodeListCombinedLength;
      }
      else
      {
        nodeList.pNodeList = NULL;
        nodeList.len = 0;
      }
    }
    else
    {
      if (0 != nodeListCombinedLength)
      {
        /** Update nodeListCombined (lifeline nodelist) with grpIdX.sourceEndpoint nodelist */
        memcpy((uint8_t* ) &nodeListCombined[nodeListCombinedLength],
               (uint8_t* ) nodeList.pNodeList, nodeList.len * sizeof(MULTICHAN_NODE_ID));
        nodeList.pNodeList = nodeListCombined;
        nodeList.len += nodeListCombinedLength;
      }

      m_lastActiveGroupId = grpId + 2;
      /*Find lifeline nodelist*/
      pTxOptionsEx = ReqNodeListTxOptions(&nodeList, nodeList.sourceEndpoint);
    }
  }

  if(NON_NULL( pTxOptionsEx ))
  {
    uint8_t i;
    ZW_DEBUG_SEND_STR("Node list: ");
    for(i = 0; i < pTxOptionsEx->list_length; i++)
    {
      ZW_DEBUG_SEND_NUM(pTxOptionsEx->pList[i].node.nodeId);
      ZW_DEBUG_SEND_BYTE('.');
      ZW_DEBUG_SEND_NUM(pTxOptionsEx->pList[i].node.endpoint);
      ZW_DEBUG_SEND_BYTE(';');
    }
    ZW_DEBUG_SEND_NL();
  }
  else
  {
    ZW_DEBUG_SEND_NL();
    ZW_DEBUG_SEND_STR("->Node list error #:");
    ZW_DEBUG_SEND_NUM(status);
    ZW_DEBUG_SEND_NL();
  }

  /*
   * S2 Multicast Group ID
   */
  if(NON_NULL(pTxOptionsEx))
  {
    pTxOptionsEx->S2_groupID = m_lastActiveGroupId + (sourceEndpoint << 4);
  }
  return pTxOptionsEx;
}
