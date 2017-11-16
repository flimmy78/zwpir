/***************************************************************************
*
* Copyright (c) 2001-2011
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Association Command Class source file
*
* Author:
*
* Last Changed By:  $Author:  $
* Revision:         $Revision:  $
* Last Changed:     $Date:  $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <ZW_tx_mutex.h>
#include <ZW_TransportLayer.h>
#include <association_plus.h>
#include <CommandClassAssociation.h>
#include <misc.h>
#include <ZW_uart_api.h>
/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_CMD_ASSOCIATION

#define ZW_DEBUG_CMD_ASSOCIATION_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_CMD_ASSOCIATION_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_CMD_ASSOCIATION_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_CMD_ASSOCIATION_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_CMD_ASSOCIATION_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_CMD_ASSOCIATION_SEND_BYTE(data)
#define ZW_DEBUG_CMD_ASSOCIATION_SEND_STR(STR)
#define ZW_DEBUG_CMD_ASSOCIATION_SEND_NUM(data)
#define ZW_DEBUG_CMD_ASSOCIATION_SEND_WORD_NUM(data)
#define ZW_DEBUG_CMD_ASSOCIATION_SEND_NL()
#endif

#define OFFSET_PARAM_2            sizeof(ZW_ASSOCIATION_GROUPINGS_REPORT_FRAME)

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
BYTE* pNodeList = NULL;
BYTE nodeListLen = 0;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

/*==============================   handleCommandClassAssociation  ============
**
**  Function:  handler for Association CC
**
**  Side effects: None
**
**--------------------------------------------------------------------------*/
void
handleCommandClassAssociation(
  BYTE  option,                    /* IN Frame header info */
  BYTE  sourceNode,                /* IN Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER *pCmd,  /* IN Payload from the received frame, the union */
  /*    should be used to access the fields */
  BYTE   cmdLength                 /* IN Number of command bytes including the command */
)
{
  switch (pCmd->ZW_AssociationGetFrame.cmd)
  {
    case ASSOCIATION_GET_V2:
      {
        BYTE groupId;
        BYTE nodeListLen = 0;
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        if(NULL != pTxBuf)
        {
          pTxBuf->ZW_AssociationReport1byteFrame.cmdClass           = COMMAND_CLASS_ASSOCIATION;
          pTxBuf->ZW_AssociationReport1byteFrame.cmd                = ASSOCIATION_REPORT_V2;

          groupId = pCmd->ZW_AssociationGetFrame.groupingIdentifier;

          if ((groupId > handleGetMaxAssociationGroups()) || (0 == groupId))
          {
            pCmd->ZW_AssociationGetFrame.groupingIdentifier = 1;
          }

          if(TRUE == handleAssociationGetnodeList(pCmd->ZW_AssociationGetFrame.groupingIdentifier, &pNodeList,&nodeListLen))
          {
            BYTE indx;
            pTxBuf->ZW_AssociationReport1byteFrame.groupingIdentifier = pCmd->ZW_AssociationGetFrame.groupingIdentifier;

            /* We force life to only support 1 association*/
            if(1 == pCmd->ZW_AssociationGetFrame.groupingIdentifier)
            {
              pTxBuf->ZW_AssociationReport1byteFrame.maxNodesSupported = 1;
              /*return only 1 association!*/
              if(1 < nodeListLen)
              {
                nodeListLen = 1;
              }
            }
            else
            {
              pTxBuf->ZW_AssociationReport1byteFrame.maxNodesSupported  = handleGetMaxNodesInGroup();
            }
            pTxBuf->ZW_AssociationReport1byteFrame.reportsToFollow = 0; //Nodes fit in one report
            ZW_DEBUG_CMD_ASSOCIATION_SEND_NL();
            ZW_DEBUG_CMD_ASSOCIATION_SEND_STR("REPORT_V2");
            for (indx = 0; indx < nodeListLen; indx++)
            {
              ZW_DEBUG_CMD_ASSOCIATION_SEND_NUM(*(pNodeList + indx));
              *(&pTxBuf->ZW_AssociationReport1byteFrame.nodeid1 + indx) = *(pNodeList + indx);
            }
            ZW_DEBUG_CMD_ASSOCIATION_SEND_NL();
          }
          else
          {
            pTxBuf->ZW_AssociationReport1byteFrame.groupingIdentifier = pCmd->ZW_AssociationGetFrame.groupingIdentifier;
            pTxBuf->ZW_AssociationReport1byteFrame.maxNodesSupported  = 0;
            pTxBuf->ZW_AssociationReport1byteFrame.reportsToFollow = 0; //Nodes fit in one report
            pTxBuf->ZW_AssociationReport1byteFrame.nodeid1 = 0;
            nodeListLen = 0;
          }
          if(!Transport_SendResponse(sourceNode,
                                    pTxBuf,
                                    sizeof(ZW_ASSOCIATION_REPORT_1BYTE_FRAME) - 1 + nodeListLen,
                                    option,
                                    ZCB_ResponseJobStatus))

          {
            /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
            FreeResponseBuffer();
          }
        }
      }
      break;

    case ASSOCIATION_SET_V2:
      AssociationAdd(pCmd->ZW_AssociationSet1byteV2Frame.groupingIdentifier,
                     &(pCmd->ZW_AssociationSet1byteV2Frame.nodeId1),
                     cmdLength - OFFSET_PARAM_2);
      break;

    case ASSOCIATION_REMOVE_V2:
      AssociationRemove(pCmd->ZW_AssociationRemove1byteV2Frame.groupingIdentifier,
                        &(pCmd->ZW_AssociationRemove1byteV2Frame.nodeId1),
                        cmdLength - OFFSET_PARAM_2);
      break;

    case ASSOCIATION_GROUPINGS_GET_V2:
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if(NULL != pTxBuf)
        {
          pTxBuf->ZW_AssociationGroupingsReportV2Frame.cmdClass = COMMAND_CLASS_ASSOCIATION;
          pTxBuf->ZW_AssociationGroupingsReportV2Frame.cmd = ASSOCIATION_GROUPINGS_REPORT_V2;
          pTxBuf->ZW_AssociationGroupingsReportV2Frame.supportedGroupings = handleGetMaxAssociationGroups();
          if(FALSE == Transport_SendResponse(sourceNode,
                                            pTxBuf,
                                            sizeof(pTxBuf->ZW_AssociationGroupingsReportV2Frame),
                                            option,
                                            ZCB_ResponseJobStatus))
          {
            /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
            FreeResponseBuffer();
          }
        }
      }
      break;
    case ASSOCIATION_SPECIFIC_GROUP_GET_V2:
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if(NULL != pTxBuf)
        {
          pTxBuf->ZW_AssociationSpecificGroupReportV2Frame.cmdClass = COMMAND_CLASS_ASSOCIATION;
          pTxBuf->ZW_AssociationSpecificGroupReportV2Frame.cmd = ASSOCIATION_SPECIFIC_GROUP_REPORT_V2;
          pTxBuf->ZW_AssociationSpecificGroupReportV2Frame.group = ApplicationGetLastActiveGroupId();
          if(FALSE == Transport_SendResponse(sourceNode,
                                            pTxBuf,
                                            sizeof(pTxBuf->ZW_AssociationSpecificGroupReportV2Frame),
                                            option,
                                            ZCB_ResponseJobStatus))
          {
            /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
            FreeResponseBuffer();
          }
        }
      }
      break;

  }
}
