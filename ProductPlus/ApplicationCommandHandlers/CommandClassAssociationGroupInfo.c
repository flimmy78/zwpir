/***************************************************************************
*
* Copyright (c) 2001-2011
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Association group info Command Class source file
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
#include <string.h>
#include <ZW_basis_api.h>
#include <ZW_TransportLayer.h>

#include "config_app.h"
#include <CommandClassAssociationGroupInfo.h>
#include <misc.h>
#include <ZW_tx_mutex.h>
#include <ZW_uart_api.h>
/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#define REPORT_ONE_GROUP 1
#define REPORT_ALL_GROUPS 2
/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
ZW_APPLICATION_TX_BUFFER *pTxBuf = NULL;
static BYTE currentGroupId;
static BYTE grInfoStatus = FALSE;
static BYTE bSourceNodeID;
static BYTE txOptions;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
void ZCB_AGIReport(BYTE txStatus);
void ZCB_AGIReportSendTimer(void);
/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
void SendAssoGroupInfoReport(void)
{
    pTxBuf = GetResponseBufferCb(ZCB_AGIReport);
    /*Check pTxBuf is free*/
    if(NULL != pTxBuf)
    {
      pTxBuf->ZW_AssociationGroupInfoReport1byteFrame.cmdClass = COMMAND_CLASS_ASSOCIATION_GRP_INFO;
      pTxBuf->ZW_AssociationGroupInfoReport1byteFrame.cmd      = ASSOCIATION_GROUP_INFO_REPORT;
      /*If thelist mode bit is set in the get frame it should be also set in the report frame.*/
      pTxBuf->ZW_AssociationGroupInfoReport1byteFrame.properties1 = (grInfoStatus == REPORT_ALL_GROUPS)? (ASSOCIATION_GROUP_INFO_REPORT_PROPERTIES1_LIST_MODE_BIT_MASK |0x01) : 0x01; /*we send one report per group*/
      GetApplGroupInfo(currentGroupId, &pTxBuf->ZW_AssociationGroupInfoReport1byteFrame.variantgroup1);
      if(FALSE == Transport_SendResponse(bSourceNodeID,
                  pTxBuf,
                  sizeof(ZW_ASSOCIATION_GROUP_INFO_REPORT_1BYTE_FRAME),
                  ZWAVE_PLUS_TX_OPTIONS,
                  ZCB_ResponseJobStatus))
      {
        /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
        FreeResponseBuffer();
        grInfoStatus = FALSE;

      }
    }
}
/*
Use this timer to delay the sending of the next AGI report after the mutex is released
Since we cannot get a new tx buffer in the call back because the mutex is reserved
*/
PCB(ZCB_AGIReportSendTimer)(void)
{
  SendAssoGroupInfoReport();
}
/*The AGI report call back we will send a report per association group
  if we seed to send AGI fro all the groups*/
PCB(ZCB_AGIReport)(BYTE txStatus)
{
  if (grInfoStatus == REPORT_ALL_GROUPS)
  {
    if (currentGroupId++ < GetApplAssoGroupsSize())
    {
      TimerStart(ZCB_AGIReportSendTimer, 1, 1);
      return;
    }
  }
  grInfoStatus = FALSE;
}

void
handleCommandClassAssociationGroupInfo(
    BYTE option,
    BYTE sourceNode,
    ZW_APPLICATION_TX_BUFFER *pCmd,
    BYTE cmdLength)
{
  BYTE length;
  BYTE groupId;
  switch (pCmd->ZW_Common.cmd)
  {
    case ASSOCIATION_GROUP_NAME_GET:
      if (0 == pCmd->ZW_AssociationGroupNameGetFrame.groupingIdentifier)
      {
        pCmd->ZW_AssociationGroupNameGetFrame.groupingIdentifier = 1;
      }
      length = GetApplGroupNameLength(pCmd->ZW_AssociationGroupNameGetFrame.groupingIdentifier);
      if (length != 0)
      {
        pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if(NULL != pTxBuf)
        {
          pTxBuf->ZW_AssociationGroupNameReport1byteFrame.cmdClass = COMMAND_CLASS_ASSOCIATION_GRP_INFO;
          pTxBuf->ZW_AssociationGroupNameReport1byteFrame.cmd      = ASSOCIATION_GROUP_NAME_REPORT;
          groupId = pCmd->ZW_AssociationGroupNameGetFrame.groupingIdentifier;
          pTxBuf->ZW_AssociationGroupNameReport1byteFrame.groupingIdentifier =  groupId;


          pTxBuf->ZW_AssociationGroupNameReport1byteFrame.lengthOfName = length;
          GetApplGroupName(&(pTxBuf->ZW_AssociationGroupNameReport1byteFrame.name1), groupId);
          if(FALSE == Transport_SendResponse(sourceNode,
                      pTxBuf,
                      sizeof(ZW_ASSOCIATION_GROUP_NAME_REPORT_1BYTE_FRAME)
                      - sizeof(BYTE)
                      + length,
                      option,
                      ZCB_ResponseJobStatus))
          {
            /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
            FreeResponseBuffer();
          }
        }
      }
      break;

    case ASSOCIATION_GROUP_INFO_GET:
      /*if we already sending reports ingore more requestes*/
      if (grInfoStatus)
        return;
      txOptions = option;
      bSourceNodeID = sourceNode;
      if (pCmd->ZW_AssociationGroupInfoGetFrame.properties1 &
          ASSOCIATION_GROUP_INFO_GET_PROPERTIES1_LIST_MODE_BIT_MASK)
      {
        /*if list mode is one then ignore groupid and report information about all the asscoication group
         one group at a time*/
         grInfoStatus =REPORT_ALL_GROUPS;
         currentGroupId = 1;
      }
      else if (pCmd->ZW_AssociationGroupInfoGetFrame.groupingIdentifier)
      {
        /*if list mode is zero and group id is not then report the association group info for the specific group*/
        grInfoStatus = REPORT_ONE_GROUP;
        currentGroupId = pCmd->ZW_AssociationGroupInfoGetFrame.groupingIdentifier;
      }
      else
      {
        /*the get frame is invalid*/
        grInfoStatus = FALSE;
      }
      if(grInfoStatus)
      {
        SendAssoGroupInfoReport();
      }
      break;

    case ASSOCIATION_GROUP_COMMAND_LIST_GET:
      groupId = pCmd->ZW_AssociationGroupCommandListGetFrame.groupingIdentifier;

      if (0 == groupId)
      {
        groupId = 1;
      }

      length = getApplGroupCommandListSize(groupId);
      if (length != 0)
      {
        pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if(NULL != pTxBuf)
        {
          pTxBuf->ZW_AssociationGroupCommandListReport1byteFrame.cmdClass = COMMAND_CLASS_ASSOCIATION_GRP_INFO;
          pTxBuf->ZW_AssociationGroupCommandListReport1byteFrame.cmd      = ASSOCIATION_GROUP_COMMAND_LIST_REPORT;
          pTxBuf->ZW_AssociationGroupCommandListReport1byteFrame.groupingIdentifier = groupId;
          pTxBuf->ZW_AssociationGroupCommandListReport1byteFrame.listLength = length;
          setApplGroupCommandList(&pTxBuf->ZW_AssociationGroupCommandListReport1byteFrame.command1, groupId);
          if(FALSE == Transport_SendResponse(sourceNode,
                      pTxBuf,
                      sizeof(ZW_ASSOCIATION_GROUP_COMMAND_LIST_REPORT_1BYTE_FRAME)
                      - sizeof(BYTE)
                      + length,
                      option,
                      ZCB_ResponseJobStatus))
          {
            /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
            FreeResponseBuffer();
          }
        }
      }
      break;

    default:
      break;
  }
}
