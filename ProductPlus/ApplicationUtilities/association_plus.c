/***************************************************************************
*
* Copyright (c) 2001-2013
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Implements functions that make easy support to
*              Association Command Class
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2013/06/21 10:15:17 $
*
****************************************************************************/

/* EEPROM binary battery sensor node layout */
/* Make sure compiler won't shuffle around with these variables,            */
/* as there are external dependencies.                                      */
/* All these variables needs to initialized, because the compiler groups    */
/* the variables into different classes for uninitialized/initialized       */
/* when ordering them. To keep them in order... Keep them all initialized.  */
#pragma ORDER

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <ZW_tx_mutex.h>
#include <ZW_uart_api.h>
#include <association_plus.h>
#include <event_util.h>
#include <ZW_TransportLayer.h>
#include <CommandClassAssociation.h>
#include <eeprom.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_ASSOCIATION
#define ZW_DEBUG_ASSOCIATION_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_ASSOCIATION_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_ASSOCIATION_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_ASSOCIATION_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_ASSOCIATION_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_ASSOCIATION_SEND_BYTE(data)
#define ZW_DEBUG_ASSOCIATION_SEND_STR(STR)
#define ZW_DEBUG_ASSOCIATION_SEND_NUM(data)
#define ZW_DEBUG_ASSOCIATION_SEND_WORD_NUM(data)
#define ZW_DEBUG_ASSOCIATION_SEND_NL()
#endif




typedef struct _ASSOCIATION_GROUP_
{
  BYTE nodeID[MAX_ASSOCIATION_IN_GROUP];
} ASSOCIATION_GROUP;


/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
/* Default values */
#define MAGIC_VALUE                 0x42


ASSOCIATION_GROUP groups[MAX_ASSOCIATION_GROUPS];

BYTE groupReportSourceNode;

BYTE indx;



/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
/*======================= ReorderGroupAfterRemove  =======================
** Function description
** reorder the association group node ID list so there are no gaps
**
** Side effects:
**
**-------------------------------------------------------------------------*/

static void
ReorderGroupAfterRemove(
  BYTE groupID,          /*the group ID to reorder*/
  BYTE emptyIndx)      /*the index of the empty field*/
{
  BYTE move;
  
  if (0 == groupID)
  {
    /* Lifeline group forced to 1 node, no reorder*/
    return;
  }
  
  for(move = emptyIndx+1; move < MAX_ASSOCIATION_IN_GROUP; move++)
  {
    groups[groupID].nodeID[move-1] = groups[groupID].nodeID[move];
    groups[groupID].nodeID[move] = 0;
  }
}

/*============================ handleAssociationGetnodeList =================
** Function description
** Deliver group number node list.
** Return status on job: TRUE/FALSE
** Side effects:
**
**-------------------------------------------------------------------------*/
BOOL
handleAssociationGetnodeList(
  BYTE groupIden,             /* IN  Group Number*/
  BYTE** ppNodeId,          /* OUT Doublepointer to list of nodes in group*/
  BYTE* pNbrOfNodes)        /* OUT Number of nodes in group*/
{
  BYTE indx;
  /*Check group number*/
  if(FALSE == handleCheckgroupIden(&groupIden))
  {
    return FALSE; /*not legal number*/
  }
  *ppNodeId = &groups[groupIden - 1].nodeID[0];
  *pNbrOfNodes = MAX_ASSOCIATION_IN_GROUP; /*default set to max*/
  for (indx = 0; indx < MAX_ASSOCIATION_IN_GROUP; indx++)
  {
    if(0 == groups[groupIden - 1].nodeID[indx])
    {
      *pNbrOfNodes = indx; /*number of nodes in list*/
      break;  /* break out of loop*/
    }
  }
  return TRUE;
}

/*============================ CheckgroupIden ===============================
** Function description
** Check if grouidentifier is legal and change *pGroupIden to 1 if
** MAX_ASSOCIATION_GROUPS = 1.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
BOOL
handleCheckgroupIden(BYTE* pGroupIden)
{
  if((GetApplAssoGroupsSize() <= (*pGroupIden - 1)) || (0 == *pGroupIden))
  {
    return FALSE; /*not legal number*/
  }
  return TRUE;
}

/*============================ AssociationGetNode ===============================
** Function description
** Get Node Id from a Association group.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
BYTE /*node Id. Return 0 if not legal*/
AssociationGetNode(
  BYTE groupIden,         /* Grouping Identifier*/
  BYTE groupIndex)             /* IN  Group index*/
{
  ZW_DEBUG_ASSOCIATION_SEND_STR("groupIden");
  ZW_DEBUG_ASSOCIATION_SEND_NUM(groupIden);
  ZW_DEBUG_ASSOCIATION_SEND_NUM(groupIndex);
  ZW_DEBUG_ASSOCIATION_SEND_NL();
  if(FALSE == handleCheckgroupIden(&groupIden))
  {
    return 0; /*not legal number*/
  }
  ZW_DEBUG_ASSOCIATION_SEND_STR("AsGNode");
  ZW_DEBUG_ASSOCIATION_SEND_NUM(groups[groupIden - 1].nodeID[groupIndex]);
  ZW_DEBUG_ASSOCIATION_SEND_NL();
  return groups[groupIden - 1].nodeID[groupIndex];
}

/*============================ handleGetMaxNodesInGroup ====================
** Function description
** Return max nodes supported in a group.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
BYTE
handleGetMaxNodesInGroup(void)
{
  return MAX_ASSOCIATION_IN_GROUP;
}

/*============================ handleGetMaxAssociationGroups ===============
** Function description
** Return max association groups..
**
** Side effects:
**
**-------------------------------------------------------------------------*/
BYTE
handleGetMaxAssociationGroups(void)
{
  return GetApplAssoGroupsSize();
}

/*============================   AssociationAdd   ======================
**    Function description
**      Adds the nodes in nodeIdP to the group
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
AssociationAdd(
  BYTE groupIden,     /*IN group number to add nodes to*/
  BYTE_P nodeIdP, /*IN pointer to list of nodes*/
  BYTE noOfNodes  /*IN number of nodes in List*/
)
{
  BYTE i;
  BYTE tempID;
  BYTE bMaxInGroup = MAX_ASSOCIATION_IN_GROUP;

  ZW_DEBUG_ASSOCIATION_SEND_STR("AssociationAdd" );
  ZW_DEBUG_ASSOCIATION_SEND_NUM(groupIden);
  ZW_DEBUG_ASSOCIATION_SEND_NUM(*nodeIdP);
  ZW_DEBUG_ASSOCIATION_SEND_NUM(noOfNodes);
  ZW_DEBUG_ASSOCIATION_SEND_NL();

  /*check group number*/
  if(FALSE == handleCheckgroupIden(&groupIden))
  {
    return;
  }

  ZW_DEBUG_ASSOCIATION_SEND_BYTE('_');

  for (i = 0; i < noOfNodes; i++)
  {
    BYTE vacant = 0xff;
    tempID = *(nodeIdP + i);
    
    if (1 == groupIden)
    {
      /* Force lifeline group to 1 index */
      bMaxInGroup = 1;
    }
      
    for (indx = 0; indx < bMaxInGroup; indx++)
    {
      if (groups[groupIden - 1].nodeID[indx])
      {
        if (groups[groupIden - 1].nodeID[indx] == tempID)
        {
          vacant = 0xFF;  /*prevent duplicated nodeID since the list can have gaps*/
          break;  /* Allready in */
        }
      }
      else
      {
        if (vacant == 0xff)
        {
          vacant = indx;
        }
      }
    }
    if (vacant != 0xff)
    {
      groups[groupIden - 1].nodeID[vacant] = tempID;
      AssociationStoreAll();
    }
  }
  CmdReceivedEvent(COMMAND_CLASS_ASSOCIATION, ASSOCIATION_SET_V2);
}

/*============================   AssociationRemove   ======================
**    Function description
**      Removes association members from specified group
**      If noOfNodes = 0 group is removed
**
**    In version 2 grouping identifier in conjunction with sequence of NodeID’s
**    are interpreted as follows:
**                                               | Grouping identifier | Number of node ID’s in list
**    ------------------------------------------------------------------------------------------------
**    Clear all node ID’s in grouping X          | 1 ≤ X ≤ N           | 0
**    Clear specified node ID’s in all groupings | 0                   | >0
**    Clear all node ID’s in all groupings       | 0                   | 0
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
AssociationRemove(
  BYTE groupIden,      /*IN groupIden number to remove nodes from*/
  BYTE_P nodeIdP,  /*IN pointer to array of nodes to remove*/
  BYTE noOfNodes   /*IN number of nodes in list*/
)
{
  BYTE i,j,indx, tempgroupIden;
  BYTE bMaxInGroup = MAX_ASSOCIATION_IN_GROUP;
  
  ZW_DEBUG_ASSOCIATION_SEND_STR("AssociationRemove ");
  ZW_DEBUG_ASSOCIATION_SEND_NUM(groupIden);
  ZW_DEBUG_ASSOCIATION_SEND_NUM(noOfNodes);
  ZW_DEBUG_ASSOCIATION_SEND_NL();

  /*Use a temp groupIden because it most not be changed*/
  tempgroupIden = groupIden;


  if ( (0 < groupIden) && (GetApplAssoGroupsSize() >= groupIden))
  {
    if(noOfNodes)
    {
      if (1 == groupIden)
      {
        /* Force lifeline group to 1 node */
        bMaxInGroup = 1;
      }
      
      /*Remove noOfNodes in list "groupIden" */
      for (i = 0; i < noOfNodes; i++)
      {
        for (indx = 0; indx < bMaxInGroup; indx++)
        {
          if (groups[groupIden-1].nodeID[indx] == *(nodeIdP+i))
          {
            BYTE move;
            ZW_DEBUG_ASSOCIATION_SEND_STR("node ");
            ZW_DEBUG_ASSOCIATION_SEND_NUM(*(nodeIdP+i));
            ZW_DEBUG_ASSOCIATION_SEND_NL();
            /*Set it to 0, if it is the last one*/
            groups[groupIden-1].nodeID[indx] = 0;
            /*Move data into the empty area*/
            ReorderGroupAfterRemove(groupIden-1, indx);
            break;  /* Found */
          }
        }
      }
    }
    else
    {
     /*Remove all nodes in list "groupIden" */
      for (indx = 0; indx < MAX_ASSOCIATION_IN_GROUP; indx++)
      {
        groups[groupIden-1].nodeID[indx] = 0;
      }
    }
    AssociationStoreAll();
  }
  else if(0 == groupIden)
  {
    if(noOfNodes)
    {
      /*Clear specified node ID's in all groupings*/
      for(j = 0; j < GetApplAssoGroupsSize(); j++)
      {
        ZW_DEBUG_ASSOCIATION_SEND_STR("group ");
        ZW_DEBUG_ASSOCIATION_SEND_NUM(j+1);
        ZW_DEBUG_ASSOCIATION_SEND_NL();
        for (i = 0; i < noOfNodes; i++)
        {
          for (indx = 0; indx < MAX_ASSOCIATION_IN_GROUP; indx++)
          {
            ZW_DEBUG_ASSOCIATION_SEND_STR("check ");
            ZW_DEBUG_ASSOCIATION_SEND_NUM(groups[j].nodeID[indx]);
            ZW_DEBUG_ASSOCIATION_SEND_BYTE(' ');
            ZW_DEBUG_ASSOCIATION_SEND_NUM(*(nodeIdP+i));
            ZW_DEBUG_ASSOCIATION_SEND_NL();
            if (groups[j].nodeID[indx] == *(nodeIdP+i))
            {
              ZW_DEBUG_ASSOCIATION_SEND_STR("remove ");
              ZW_DEBUG_ASSOCIATION_SEND_NUM(groups[j].nodeID[indx]);
              ZW_DEBUG_ASSOCIATION_SEND_NL();
              groups[j].nodeID[indx] = 0;
              ReorderGroupAfterRemove(j, indx);
              break;  /* Found */
            }
          }
        }
      }
      AssociationStoreAll();
    }
    else
    {
      /*Clear all node ID's in all groupings*/
      AssociationInit(TRUE);
    }
  }
  CmdReceivedEvent(COMMAND_CLASS_ASSOCIATION, ASSOCIATION_REMOVE);
}

/*============================   AssociationStoreAll   ======================
**    Function description
**      Stores all groups in external Nonvolatile memory. Used by association.c
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
AssociationStoreAll(void)
{
  ZW_DEBUG_ASSOCIATION_SEND_BYTE('D');
  ZW_DEBUG_ASSOCIATION_SEND_BYTE('2');
  MemoryPutBuffer((WORD)&nvmApplDescriptor.EEOFFSET_ASSOCIATION_START_far, (BYTE *)&groups[0], ASSOCIATION_SIZE, NULL);
}

/*============================   AssociationClearAll   ======================
**    Function description
**      Clears the Association area in Nonvolatile memory. Used by association.c
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
AssociationClearAll(void)
{
  ZW_DEBUG_ASSOCIATION_SEND_BYTE('D');
  ZW_DEBUG_ASSOCIATION_SEND_BYTE('1');
  MemoryPutBuffer((WORD)&nvmApplDescriptor.EEOFFSET_ASSOCIATION_START_far, NULL, ASSOCIATION_SIZE, NULL);
  MemoryPutByte((WORD)&nvmApplDescriptor.EEOFFSET_ASSOCIATION_MAGIC_far, MAGIC_VALUE); /* Now ASSOCIATION should be OK */
}

/*============================   AssociationInit   ==========================
**    Function description
**      Reads the groups stored in the Nonvolatile memory
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
AssociationInit(BOOL forceClearMem)
{
  ZW_DEBUG_ASSOCIATION_SEND_BYTE('D');
  ZW_DEBUG_ASSOCIATION_SEND_BYTE('0');
  if ((MemoryGetByte((WORD)&nvmApplDescriptor.EEOFFSET_ASSOCIATION_MAGIC_far) != MAGIC_VALUE) ||
      (TRUE == forceClearMem))
  {
    /* Clear it */
    AssociationClearAll();
  }
  MemoryGetBuffer((WORD)&nvmApplDescriptor.EEOFFSET_ASSOCIATION_START_far, (BYTE *)&groups[0], ASSOCIATION_SIZE);
}

/*=======================   AssociationGetLifeLineNodeID   ========================
**    Function description
**      Reads the nodeID asscoiated to the lifeLine group
**
**    Side effects:
**
**------------------------------------------------------------------------------*/
BYTE                              /*RET the nodeID of the lifeline node, 0xFF if not associated*/
AssociationGetLifeLineNodeID(void)
{
  /*get the nodeID associated to the lifeline group*/
  return (groups[0].nodeID[0])?groups[0].nodeID[0]:0xFF;
}

