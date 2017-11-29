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
#ifdef ZW_DEBUG_CCAGI
#define ZW_DEBUG_CCAGI_SEND_BYTE(data)      ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_CCAGI_SEND_STR(STR)        ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_CCAGI_SEND_NUM(data)       ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_CCAGI_SEND_WORD_NUM(data)  ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_CCAGI_SEND_NL()            ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_CCAGI_SEND_BYTE(data)
#define ZW_DEBUG_CCAGI_SEND_STR(STR)
#define ZW_DEBUG_CCAGI_SEND_NUM(data)
#define ZW_DEBUG_CCAGI_SEND_WORD_NUM(data)
#define ZW_DEBUG_CCAGI_SEND_NL()
#endif

#define REPORT_ONE_GROUP 1
#define REPORT_ALL_GROUPS 2
/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
ZW_APPLICATION_TX_BUFFER *pTxBuf = NULL;
static BYTE currentGroupId;
static BYTE associationGroupInfoGetEndpoint = 0;
static BYTE grInfoStatus = FALSE;
static RECEIVE_OPTIONS_TYPE_EX rxOptionsEx;


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
void ZCB_AGIReport(BYTE txStatus);
void ZCB_AGIReportSendTimer(void);

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
void SendAssoGroupInfoReport(RECEIVE_OPTIONS_TYPE_EX *rxOpt)
{
  ZW_DEBUG_CCAGI_SEND_NL();
  ZW_DEBUG_CCAGI_SEND_STR("SendAssoGroupInfoReport");
  ZW_DEBUG_CCAGI_SEND_NUM(currentGroupId);

    pTxBuf = GetResponseBufferCb(ZCB_AGIReport);
    //pTxBuf = GetRequestBuffer(ZCB_AGIReport);
    /*Check pTxBuf is free*/
    if( NON_NULL( pTxBuf ) )
    {
      TRANSMIT_OPTIONS_TYPE_SINGLE_EX *txOptionsEx;
      RxToTxOptions(rxOpt, &txOptionsEx);
      pTxBuf->ZW_AssociationGroupInfoReport1byteFrame.cmdClass = COMMAND_CLASS_ASSOCIATION_GRP_INFO;
      pTxBuf->ZW_AssociationGroupInfoReport1byteFrame.cmd      = ASSOCIATION_GROUP_INFO_REPORT;
      /*If thelist mode bit is set in the get frame it should be also set in the report frame.*/
      pTxBuf->ZW_AssociationGroupInfoReport1byteFrame.properties1 = (grInfoStatus == REPORT_ALL_GROUPS)? (ASSOCIATION_GROUP_INFO_REPORT_PROPERTIES1_LIST_MODE_BIT_MASK |0x01) : 0x01; /*we send one report per group*/
      GetApplGroupInfo(currentGroupId, rxOpt->destNode.endpoint, &pTxBuf->ZW_AssociationGroupInfoReport1byteFrame.variantgroup1);
      if (ZW_TX_IN_PROGRESS != Transport_SendResponseEP( (BYTE *)pTxBuf,
                  sizeof(ZW_ASSOCIATION_GROUP_INFO_REPORT_1BYTE_FRAME),
                  txOptionsEx,
                  ZCB_ResponseJobStatus))
      {
        ZW_DEBUG_CCAGI_SEND_NL();
        ZW_DEBUG_CCAGI_SEND_STR("TX error");
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
  SendAssoGroupInfoReport(&rxOptionsEx);
}


/*The AGI report call back we will send a report per association group
  if we seed to send AGI fro all the groups*/
PCB(ZCB_AGIReport)(BYTE txStatus)
{
  UNUSED(txStatus);
  ZW_DEBUG_CCAGI_SEND_NL();
  ZW_DEBUG_CCAGI_SEND_STR("ZCB_AGIReport");
  ZW_DEBUG_CCAGI_SEND_NUM(grInfoStatus);
  if (grInfoStatus == REPORT_ALL_GROUPS)
  {
    if (currentGroupId++ < GetApplAssoGroupsSize(associationGroupInfoGetEndpoint))
    {
      TimerStart(ZCB_AGIReportSendTimer, 1, 1);
      return;
    }
  }
  grInfoStatus = FALSE;
}

received_frame_status_t
handleCommandClassAssociationGroupInfo(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength)
{
  BYTE length;
  BYTE groupID;
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX *txOptionsEx;
  uint8_t groupNameLength;
  UNUSED(cmdLength);
  switch (pCmd->ZW_Common.cmd)
  {
    case ASSOCIATION_GROUP_NAME_GET:

      if (TRUE == Check_not_legal_response_job(rxOpt))
      {
        // Get/Report do not support bit addressing.
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      pTxBuf = GetResponseBuffer();
      if (IS_NULL(pTxBuf))
      {
        // The buffer is not free :(
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      if (3 != cmdLength)
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      pTxBuf->ZW_AssociationGroupNameReport1byteFrame.cmdClass = COMMAND_CLASS_ASSOCIATION_GRP_INFO;
      pTxBuf->ZW_AssociationGroupNameReport1byteFrame.cmd      = ASSOCIATION_GROUP_NAME_REPORT;

      groupID = pCmd->ZW_AssociationGroupNameGetFrame.groupingIdentifier;
      pTxBuf->ZW_AssociationGroupNameReport1byteFrame.groupingIdentifier = groupID;

      groupNameLength = GetApplGroupName(
          (char *)&(pTxBuf->ZW_AssociationGroupNameReport1byteFrame.name1),
          groupID,
          rxOpt->destNode.endpoint);

      pTxBuf->ZW_AssociationGroupNameReport1byteFrame.lengthOfName = groupNameLength;

      RxToTxOptions(rxOpt, &txOptionsEx);
      if (ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
          (BYTE *)pTxBuf,
          sizeof(ZW_ASSOCIATION_GROUP_NAME_REPORT_1BYTE_FRAME) - sizeof(BYTE) + groupNameLength,
          txOptionsEx,
          ZCB_ResponseJobStatus))
      {
        /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
        FreeResponseBuffer();
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;

    case ASSOCIATION_GROUP_INFO_GET:
      ZW_DEBUG_CCAGI_SEND_NL();
      ZW_DEBUG_CCAGI_SEND_STR("ASSOCIATION_GROUP_INFO_GET");
      ZW_DEBUG_CCAGI_SEND_NUM(rxOpt->destNode.endpoint);

      if (TRUE == Check_not_legal_response_job(rxOpt))
      {
        /*Get/Report do not support endpoint bit-addressing, free transmit-buffer pTxBuf by cleaing mutex */
        FreeResponseBuffer();
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      /*if we already sending reports ingore more requestes*/
      if (grInfoStatus)
      {
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      memcpy((BYTE_P)&rxOptionsEx, (BYTE_P)rxOpt, sizeof(RECEIVE_OPTIONS_TYPE_EX));
      if (pCmd->ZW_AssociationGroupInfoGetFrame.properties1 &
          ASSOCIATION_GROUP_INFO_GET_PROPERTIES1_LIST_MODE_BIT_MASK)
      {
        /*if list mode is one then ignore groupid and report information about all the asscoication group
         one group at a time*/
         grInfoStatus =REPORT_ALL_GROUPS;
         currentGroupId = 1;
         associationGroupInfoGetEndpoint = rxOpt->destNode.endpoint;
      }
      else if (pCmd->ZW_AssociationGroupInfoGetFrame.groupingIdentifier)
      {
        /*if list mode is zero and group id is not then report the association group info for the specific group*/
        grInfoStatus = REPORT_ONE_GROUP;
        currentGroupId = pCmd->ZW_AssociationGroupInfoGetFrame.groupingIdentifier;
        associationGroupInfoGetEndpoint = rxOpt->destNode.endpoint;
      }
      else
      {
        /*the get frame is invalid*/
        grInfoStatus = FALSE;
      }
      ZW_DEBUG_CCAGI_SEND_NL();
      ZW_DEBUG_CCAGI_SEND_STR("grInfoStatus: ");
      ZW_DEBUG_CCAGI_SEND_NUM(grInfoStatus);

      if (grInfoStatus)
      {
        SendAssoGroupInfoReport(&rxOptionsEx);
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;

    case ASSOCIATION_GROUP_COMMAND_LIST_GET:
      if(TRUE == Check_not_legal_response_job(rxOpt))
      {
        /*Get/Report do not support endpoint bit-addressing, free transmit-buffer pTxBuf by cleaing mutex */
        FreeResponseBuffer();
        return RECEIVED_FRAME_STATUS_FAIL;
      }

      length = GetApplGroupCommandListSize(pCmd->ZW_AssociationGroupCommandListGetFrame.groupingIdentifier, rxOpt->destNode.endpoint);
      if (length != 0)
      {
        pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if( NON_NULL( pTxBuf ) )
        {
          TRANSMIT_OPTIONS_TYPE_SINGLE_EX *txOptionsEx;
          RxToTxOptions(rxOpt, &txOptionsEx);
          pTxBuf->ZW_AssociationGroupCommandListReport1byteFrame.cmdClass = COMMAND_CLASS_ASSOCIATION_GRP_INFO;
          pTxBuf->ZW_AssociationGroupCommandListReport1byteFrame.cmd      = ASSOCIATION_GROUP_COMMAND_LIST_REPORT;
          groupID = pCmd->ZW_AssociationGroupCommandListGetFrame.groupingIdentifier;
          pTxBuf->ZW_AssociationGroupCommandListReport1byteFrame.groupingIdentifier = groupID;
          pTxBuf->ZW_AssociationGroupCommandListReport1byteFrame.listLength = length;
          GetApplGroupCommandList(&pTxBuf->ZW_AssociationGroupCommandListReport1byteFrame.command1, groupID, rxOpt->destNode.endpoint);

          if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP( (BYTE *)pTxBuf,
                      sizeof(ZW_ASSOCIATION_GROUP_COMMAND_LIST_REPORT_1BYTE_FRAME)
                      - sizeof(BYTE)
                      + length,
                      txOptionsEx,
                      ZCB_ResponseJobStatus))
          {
            /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
            FreeResponseBuffer();
          }
        }
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}
