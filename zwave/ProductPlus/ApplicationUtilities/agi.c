/***************************************************************************
*
* Copyright (c) 2001-2014
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Some nice descriptive description.
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2014/05/28 15:04:02 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <agi.h>
#include "config_app.h"
#include <ZW_mem_api.h>
#include <ZW_string.h>
#include <ZW_uart_api.h>
#include <association_plus.h>
#include <ZW_classcmd.h>
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



#if (APP_NUMBER_OF_ENDPOINTS == 0)
#define NUMBER_OF_ENDPOINTS 1
#else
#define NUMBER_OF_ENDPOINTS APP_NUMBER_OF_ENDPOINTS
#endif

typedef struct ___AGI_GROUP_
{
  AGI_PROFILE profile; /**< AGI profile for type: ASSOCIATION_GROUP_INFO_REPORT_PROFILE_...*/
  CMD_CLASS_GRP cmdGrp; /**< AGI Profile cmd class group*/
  BYTE groupName[42]; /**< AGI Profile group-name UTF-8 format*/
} __AGI_GROUP;


typedef struct _AGI_LIFELINE_
{
  BYTE grpName[42];
  CMD_CLASS_GRP* pCmdGrpList;
  BYTE listSize;
} AGI_LIFELINE;

typedef struct _AGI_TABLE_EP_
{
  __AGI_GROUP* pTable;
  BYTE tableSize;
} AGI_TABLE_EP;

typedef struct _AGI_TABLE_
{
  AGI_LIFELINE lifeLine;
  AGI_TABLE_EP tableEndpoint[NUMBER_OF_ENDPOINTS];
} AGI_TABLE;
/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
const char GroupName[] = "Lifeline";


AGI_TABLE myAgi;
AGI_PROFILE m_InAgiProfile;
BYTE m_groupingIdentifier =0;
BYTE m_lastActiveGroupId = 1;
BOOL m_seachLifeLine = TRUE;
/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

void
AGI_LifeLineGroupSetup( CMD_CLASS_GRP* pCmdGrpList, BYTE listSize)
{
  memset((BYTE *)&myAgi, 0x00, sizeof(myAgi));
#ifndef __51__
  memset((BYTE *)&myAgi, 0x00, sizeof(myAgi));
#endif /*__51__*/
  myAgi.lifeLine.pCmdGrpList = pCmdGrpList;
  myAgi.lifeLine.listSize = listSize;
  memcpy(myAgi.lifeLine.grpName, GroupName, ZW_strlen((BYTE *)GroupName));
}

void
AGI_ResourceGroupSetup(AGI_GROUP* pTable, BYTE tableSize, BYTE endpoint)
{
  ZW_DEBUG_AGI_SEND_STR("AGI_ResourceGroupSetup tableSize ");
  ZW_DEBUG_AGI_SEND_NUM(tableSize);
  ZW_DEBUG_AGI_SEND_NL();
  // code is handling enpoints as from 0,1... User can handle endpoint as 0 (no endpoints)or 1,2..
  if( 0 != endpoint)
  {
    endpoint--;
  }
  if(NUMBER_OF_ENDPOINTS >= endpoint)
  {
    myAgi.tableEndpoint[endpoint].pTable = (__AGI_GROUP*)pTable;
    myAgi.tableEndpoint[endpoint].tableSize = tableSize;
  }
}

/*========================   GetApplGroupName  =========================
**   set  Application specific Group Name
**   return none
**
**   Side effects: none
**--------------------------------------------------------------------------*/
void
GetApplGroupName(
    BYTE* pGroupName,
	BYTE groupId
)
{
  ZW_DEBUG_AGI_SEND_STR("GetApplGroupName");
  ZW_DEBUG_AGI_SEND_NL();
  if (groupId == 1)
  {
    memcpy(pGroupName, myAgi.lifeLine.grpName, ZW_strlen(myAgi.lifeLine.grpName));
  }
  else{
    /*Endpoint is hardcoded to 0 for this version!!*/
    memcpy(pGroupName, myAgi.tableEndpoint[0].pTable[groupId-2].groupName, ZW_strlen(myAgi.tableEndpoint[0].pTable[groupId-2].groupName));
  }
}

/*========================   GetApplGroupNameLength  =========================
**   get Application specific Group Name Length
**   return size of Group Name Length
**
**   Side effects: none
**--------------------------------------------------------------------------*/
BYTE
GetApplGroupNameLength(
	BYTE groupId
)
{
  ZW_DEBUG_AGI_SEND_STR("GetApplGroupNameLength");
  ZW_DEBUG_AGI_SEND_NL();

  if(groupId < myAgi.tableEndpoint[0].tableSize + 2)
  {

    if(1 == groupId)
    {
      return ZW_strlen(myAgi.lifeLine.grpName);
    }
    else
    {
      /*Endpoint is hardcoded to 0 for this version!!*/
      return ZW_strlen(myAgi.tableEndpoint[0].pTable[groupId-2].groupName);
    }
  }
  ZW_DEBUG_AGI_SEND_STR("Error length");
  ZW_DEBUG_AGI_SEND_NL();
  return 0;
}

/*========================   GetApplAssoGroupsSize  ===========
**   get number of the application association groups
**   return none
**
**   Side effects: none
**--------------------------------------------------------------------------*/
BYTE
GetApplAssoGroupsSize(
  void
)
{
  ZW_DEBUG_AGI_SEND_STR("GetApplAssoGroupsSize");
  ZW_DEBUG_AGI_SEND_NL();
  /*Endpoint is hardcoded to 0 for this version!!*/
  return 1 + myAgi.tableEndpoint[0].tableSize; /* Lifeline group + grouptable size.*/
}

/*========================   GetApplGroupInfo  ===========
**   set Application specific Group Info Report
**   return none
**
**   Side effects: none
**--------------------------------------------------------------------------*/
void
GetApplGroupInfo(
  BYTE groupId,
  VG_ASSOCIATION_GROUP_INFO_REPORT_VG* report
)
{
  ZW_DEBUG_AGI_SEND_STR("GetApplGroupInfo ID ");
  ZW_DEBUG_AGI_SEND_NUM(groupId);
  ZW_DEBUG_AGI_SEND_NL();
  if(groupId >= myAgi.tableEndpoint[0].tableSize + 2)
  {
    /*Not legal groupId!*/
    report->groupingIdentifier = 0;
    report->mode = 0;                         /**/
    report->profile1 = 0;                     /* MSB */
    report->profile2 = 0;                     /* LSB */
    report->reserved = 0;                     /**/
    report->eventCode1 = 0;                   /* MSB */
    report->eventCode2 = 0;                   /* LSB */
  }

  if(1 == groupId)
  {
    /*Report all association groups in one message!*/
    report->groupingIdentifier = groupId;
    report->mode = 0;                         /**/
    report->profile1 = ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL; /* MSB */
    report->profile2 = 0x01; /* LSB */
    report->reserved = 0;                     /**/
    report->eventCode1 = 0;                   /* MSB */
    report->eventCode2 = 0;                   /* LSB */
  }
  else
  {
    report->groupingIdentifier = groupId;
    report->mode = 0;                         /**/
    report->profile1 = (myAgi.tableEndpoint[0].pTable[groupId-2].profile.profile_MS); /* MSB */
    report->profile2 = ( myAgi.tableEndpoint[0].pTable[groupId-2].profile.profile_LS); /* LSB */
    report->reserved = 0;                     /**/
    report->eventCode1 = 0;                   /* MSB */
    report->eventCode2 = 0;                   /* LSB */

    ZW_DEBUG_AGI_SEND_STR("** profile ");
    ZW_DEBUG_AGI_SEND_WORD_NUM(myAgi.tableEndpoint[0].pTable[groupId-2].profile);
    ZW_DEBUG_AGI_SEND_NL();
  }

}

/*========================   setApplGroupCommandList  ===========
**   set Application specific Group Command List
**   return none
**
**   Side effects: none
**--------------------------------------------------------------------------*/
void
setApplGroupCommandList(
  BYTE* pGroupList,
  BYTE groupId
)
{
  ZW_DEBUG_AGI_SEND_STR("setApplGroupCommandList");
  ZW_DEBUG_AGI_SEND_NL();
  if(groupId >= myAgi.tableEndpoint[0].tableSize + 2)
  {
    /*Not legal groupId!*/
    * pGroupList = 0x00;
    return;
  }

  if (groupId == 1)
  {
    memcpy(pGroupList, myAgi.lifeLine.pCmdGrpList, myAgi.lifeLine.listSize * sizeof(CMD_CLASS_GRP));
  }
  else
  {
    memcpy(pGroupList, &myAgi.tableEndpoint[0].pTable[groupId-2].cmdGrp, sizeof(CMD_CLASS_GRP));
  }
}

/*========================   getApplGroupCommandListSize  ===========
**    Application specific Group Command List Size getter
**   return size of GroupCommandList
**
**   Side effects: none
**--------------------------------------------------------------------------*/
BYTE
getApplGroupCommandListSize(
  BYTE groupId
)
{
  BYTE size = 0;
  ZW_DEBUG_AGI_SEND_STR("getApplGroupCommandListSize");
  ZW_DEBUG_AGI_SEND_NL();
  if(groupId >= myAgi.tableEndpoint[0].tableSize + 2)
  {
    /*Not legal groupId!*/
    return 0;
  }

  if (groupId == 1)
  {
    size =  myAgi.lifeLine.listSize * sizeof(CMD_CLASS_GRP);;
  }
  else
  {
    size =  sizeof(CMD_CLASS_GRP);
  }
  return size;
}


/*============================ SetSpecificGroupId ===============================
** Function description
** Set specific group Id. THis function is intended to assist the association
** of multi-button wall controller devices so that individual buttons can be
** mapped to different target devices. Please read "Z-Wave command Class
** specification" chapter "Association Specific Group Get Command".
**
** Side effects:
**
**-------------------------------------------------------------------------*/
BOOL
AGI_SpecificGroupIdSet( AGI_PROFILE profile, BYTE endpoint)
{
  BOOL status = FALSE;
  CMD_CLASS_GRP* pCmdGrp;
  AGI_NodeIdListInit(NULL, (AGI_PROFILE *)&profile);

  ZW_DEBUG_AGI_SEND_STR("SetSpecificGroupId");

  while(TRUE == AGI_ResourceNodeIdListLookup( profile, endpoint, &pCmdGrp, NULL, NULL))
  {
    /* Find first identifier*/
    if(FALSE == status)
    {
      status = TRUE;
      m_lastActiveGroupId = m_groupingIdentifier + 1;
    }

    /*Check if command class is CC-basic*/
    if(COMMAND_CLASS_BASIC == pCmdGrp->cmdClass)
    {
      /* We want group identifier to be command class Basic... else use the
         first found identifier*/
      m_lastActiveGroupId = m_groupingIdentifier + 1;
      break;
    }
  }
  ZW_DEBUG_AGI_SEND_NUM(m_lastActiveGroupId);
  ZW_DEBUG_AGI_SEND_NL();
  return status;
}


/*============================ ApplicationGetLastActiveGroupId ==============
** Function description
** Report the current active group.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
BYTE
ApplicationGetLastActiveGroupId(void)
{
  ZW_DEBUG_AGI_SEND_STR("ApplicationGetLastActiveGroupId");
  ZW_DEBUG_AGI_SEND_NL();
  return m_lastActiveGroupId;
}

/*============================ SearchNextProfileId ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
BOOL
SearchNextProfileId(AGI_PROFILE profile, BYTE endpoint )
{

  ZW_DEBUG_AGI_SEND_STR("SearchNextProfileId");
  ZW_DEBUG_AGI_SEND_WORD_NUM( profile);
  ZW_DEBUG_AGI_SEND_BYTE(' ');
  ZW_DEBUG_AGI_SEND_NUM(endpoint);
  ZW_DEBUG_AGI_SEND_NL();

	/*Check for lifeline*/
	if( (ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL == profile.profile_MS) && (ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_LIFELINE == profile.profile_LS))
  {
      ZW_DEBUG_AGI_SEND_STR("Function do not support lifeline!");
      ZW_DEBUG_AGI_SEND_NL();
    return FALSE;
  }
  else
  {
    if(NULL == myAgi.tableEndpoint[endpoint].pTable)
    {
      return FALSE;
    }

    while((profile.profile_MS != myAgi.tableEndpoint[endpoint].pTable[m_groupingIdentifier].profile.profile_MS) ||
          (profile.profile_LS != myAgi.tableEndpoint[endpoint].pTable[m_groupingIdentifier].profile.profile_LS))
    {
      m_groupingIdentifier++;
      if(m_groupingIdentifier >= myAgi.tableEndpoint[endpoint].tableSize)
      {
        /* no more profiles*/
        return FALSE;
      }
    }
  }
  return TRUE;
}

/*============================ GetNodeIdListFirst ===============================
** Function description
** Get Lifeline node list.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
BOOL
AGI_LifeLineNodeIdListLookup( CMD_CLASS_GRP** ppCmdGrp,BYTE* pListSize, BYTE** ppNodeList, BYTE* pNodeListLen)
{
  ZW_DEBUG_AGI_SEND_STR("AGI_LifeLineNodeIdListLookup");
  *ppCmdGrp = myAgi.lifeLine.pCmdGrpList;
  *pListSize = myAgi.lifeLine.listSize;

  if(FALSE == handleAssociationGetnodeList(1, ppNodeList, pNodeListLen))
  {
    *ppCmdGrp = NULL;
    *pListSize = 0;
    *ppNodeList = NULL;
    *pNodeListLen = 0;
    return FALSE;
  }

  if(0 == *pNodeListLen)
  {
    ZW_DEBUG_AGI_SEND_STR("*** FAIL NO association!! ***");
    ZW_DEBUG_AGI_SEND_NL();
    return FALSE;
  }

  ZW_DEBUG_AGI_SEND_NL();
  return TRUE;

}

/*============================ GetNodeIdListFirst ===============================
** Function description
** Get node list out from AGI profile. This reset the search in AGI table.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
AGI_NodeIdListInit(NODE_LIST* pNextNode, AGI_PROFILE* pInAgiProfile)
{
  if(NULL != pNextNode)
  {
    pNextNode->pNodeList = NULL;
    pNextNode->len = 0;
    pNextNode->pCurrentNode = NULL;
    pNextNode->pCurrentCmdGrp = NULL;
  }
  m_InAgiProfile = *pInAgiProfile;
  m_groupingIdentifier = 0;
  m_seachLifeLine = TRUE;
}


/*============================ AGI_ResourceNodeIdListLookup ===============================
** Function description
** Get next node list out from AGI profile.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
BOOL
AGI_ResourceNodeIdListLookup( AGI_PROFILE profile,  BYTE endpoint, CMD_CLASS_GRP** ppCmdGrp,  BYTE** ppNodeList, BYTE* pNodeListLen)
{
  BOOL status;
  ZW_DEBUG_AGI_SEND_STR("AGI_ResourceNodeIdListLookup");
  ZW_DEBUG_AGI_SEND_WORD_NUM(profile);
  ZW_DEBUG_AGI_SEND_NL();
  *pNodeListLen = 0;

  /*1. Find group-ID in AGI table*/
  do{
    if (status = SearchNextProfileId(profile, endpoint) == TRUE)
    {
      if(NULL != ppCmdGrp){
        *ppCmdGrp = &(myAgi.tableEndpoint[endpoint].pTable[m_groupingIdentifier].cmdGrp);
      }

  	  /*2. Use group-Id to find node list in association module. remember to add 2 to group-ID*/
  	  /* Do not search after enpoints ff ppNodeList is NULL!*/
  	  if( NULL != ppNodeList && NULL != pNodeListLen)
  	  {
    	  status = handleAssociationGetnodeList(m_groupingIdentifier+2, ppNodeList, pNodeListLen);
    	  if(0 == *pNodeListLen)
    	  {
          ZW_DEBUG_AGI_SEND_STR(">handleAssociationGetnodeList FALSE");
          ZW_DEBUG_AGI_SEND_NL();
    	    status = FALSE;
    	  }
    	}
  	  m_groupingIdentifier++; /* We have now read data and are ready and need to increment parameter*/
  	}
  	else{
  	  m_groupingIdentifier++; /* We have now read data and are ready and need to increment parameter*/
  	}
  }while((status == FALSE) && (myAgi.tableEndpoint[endpoint].tableSize > m_groupingIdentifier) );


  if(FALSE == status)
  {
    *ppNodeList = NULL;
    *pNodeListLen = 0;
    return FALSE;
  }
  return TRUE;
}

/*============================ AGI_NodeIdListGetNext ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
BOOL
AGI_NodeIdListGetNext( NODE_LIST* pNextNode)
{
  BYTE endpoint = 0; //Apps do not have support for endpoints.
  ZW_DEBUG_AGI_SEND_STR("NNListAgiP ");
  ZW_DEBUG_AGI_SEND_NUM(m_seachLifeLine);
  ZW_DEBUG_AGI_SEND_STR(" profile ");
  ZW_DEBUG_AGI_SEND_WORD_NUM(m_InAgiProfile);
  ZW_DEBUG_AGI_SEND_NL();

  if((pNextNode->pCurrentNode != NULL) && (pNextNode->pCurrentNode >=  pNextNode->pNodeList) &&
    (pNextNode->pCurrentNode <  (pNextNode->pNodeList + pNextNode->len - 1)))
  {
    /*Next node in Lifeline association list*/
    pNextNode->pCurrentNode++;
    ZW_DEBUG_AGI_SEND_STR("Next node in Lifeline");
    ZW_DEBUG_AGI_SEND_NL();
    return TRUE;
  }
  else if((pNextNode->pCurrentNode != NULL) && pNextNode->pCurrentNode >=  (pNextNode->pNodeList + pNextNode->len - 1))
  {
    /*search for more profile CC in lifeline*/
    ZW_DEBUG_AGI_SEND_STR("search for more profile CC");
    ZW_DEBUG_AGI_SEND_NL();
    pNextNode->pCurrentNode = NULL;
  }


  /*Send over Lifeline*/
  if( TRUE == m_seachLifeLine)
  {

    BYTE listSize, i;
    BOOL result = FALSE;

    /*1. Find group-ID in AGI resource table*/
    if( (ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL == m_InAgiProfile.profile_MS) &&
        (ASSOCIATION_GROUP_INFO_REPORT_PROFILE_GENERAL_LIFELINE == m_InAgiProfile.profile_LS))
    {
      //Lifeline profile is not supported!
      return TRUE;
    }
    

    /*1. Find group-ID in AGI resource table*/
    if ( result = SearchNextProfileId( m_InAgiProfile, endpoint) == TRUE)
    {
      if( COMMAND_CLASS_BASIC == myAgi.tableEndpoint[endpoint].pTable[m_groupingIdentifier].cmdGrp.cmdClass)
      {
         /* We do not use basic cc in ligeline!*/
         m_groupingIdentifier++;
         result = SearchNextProfileId(m_InAgiProfile, endpoint);
      }
      ZW_DEBUG_AGI_SEND_STR("SearchNextProfileId ");
      ZW_DEBUG_AGI_SEND_NUM(result);
      ZW_DEBUG_AGI_SEND_NL();

      if(TRUE == result)
      {
        CMD_CLASS_GRP profileGrp;
        profileGrp.cmdClass = myAgi.tableEndpoint[endpoint].pTable[m_groupingIdentifier].cmdGrp.cmdClass;
        profileGrp.cmd = myAgi.tableEndpoint[endpoint].pTable[m_groupingIdentifier].cmdGrp.cmd;
        /*We have read AGI profile list. Increase it for next seach*/
        m_groupingIdentifier++;

        ZW_DEBUG_AGI_SEND_STR("Found profile CC ");
        ZW_DEBUG_AGI_SEND_NUM(profileGrp.cmdClass);
        ZW_DEBUG_AGI_SEND_NL();

        if(TRUE == AGI_LifeLineNodeIdListLookup( &(pNextNode->pCurrentCmdGrp), &listSize, &(pNextNode->pNodeList), &(pNextNode->len)))
        {
          /*Check cmdGroup is in CmdGrp-list*/
          for( i = 0; i < listSize;i++)
          {
            if((profileGrp.cmdClass == pNextNode->pCurrentCmdGrp[i].cmdClass) && (profileGrp.cmd == pNextNode->pCurrentCmdGrp[i].cmd))
            {
              pNextNode->pCurrentCmdGrp = &(pNextNode->pCurrentCmdGrp[i]);
              /*We found commad group and ready to use the association node list (pNextNode->pNodeList).*/
              pNextNode->pCurrentNode = pNextNode->pNodeList;
              ZW_DEBUG_AGI_SEND_STR("Validate profile in lifeline node ");
              ZW_DEBUG_AGI_SEND_NUM(*pNextNode->pCurrentNode);
              ZW_DEBUG_AGI_SEND_NL();
              return TRUE;
            }
          }
          m_seachLifeLine = FALSE;
          m_groupingIdentifier = 0;
        }
        else{
          m_seachLifeLine = FALSE;
          m_groupingIdentifier = 0;
        }
      }
      else
      {
        m_seachLifeLine = FALSE;
        m_groupingIdentifier = 0;
      }

    }
    else{
      /* no more profile CC in lifeline. Search now in profile list*/
      m_seachLifeLine = FALSE;
      m_groupingIdentifier = 0;
    }
  }

  if(TRUE == AGI_ResourceNodeIdListLookup(m_InAgiProfile, 0, &(pNextNode->pCurrentCmdGrp), &(pNextNode->pNodeList), &(pNextNode->len)))
  {
    pNextNode->pCurrentNode = pNextNode->pNodeList;
    return TRUE;
  }
  pNextNode->pNodeList = NULL;
  pNextNode->pCurrentNode = NULL;
  pNextNode->len = 0;
  return FALSE;
}

