/**
 * @file CommandClassMultiChanAssociation.c
 * @brief Command Class Multi Channel Association.
 * @author Thomas Roll
 * @author Christian Salmony Olsen
 * @copyright Copyright (c) 2001-2016
 * Sigma Designs, Inc.
 * All Rights Reserved
 * @details Handles commands in the Multi Channel Association Command Class.
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <CommandClassMultiChanAssociation.h>
#include <ZW_tx_mutex.h>
#include <ZW_TransportLayer.h>
#include <association_plus.h>
#include <misc.h>
#include <ZW_uart_api.h>
#include <ZW_mem_api.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_CC_MULTICHAN_ASSOCIATION
#define CCMCA_(x) x
#else
#define CCMCA_(x)
#endif
#define _CCMCA_(x) /*outcommon debug print*/


#define OFFSET_PARAM_1                        0x02

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

received_frame_status_t
handleCommandClassMultiChannelAssociation(
    RECEIVE_OPTIONS_TYPE_EX *rxOpt,
    ZW_APPLICATION_TX_BUFFER *pCmd,
    BYTE cmdLength)
{
  ZW_APPLICATION_TX_BUFFER * pTxBuf;
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
  uint8_t txResult;

  if (TRUE == Check_not_legal_response_job(rxOpt))
  {
    /*Do not support endpoint bit-addressing */
    return RECEIVED_FRAME_STATUS_FAIL;
  }

  switch (pCmd->ZW_Common.cmd)
  {
    case MULTI_CHANNEL_ASSOCIATION_GET_V2:
      {
        uint8_t outgoingFrameLength;

        pTxBuf = GetResponseBuffer();
        if (IS_NULL(pTxBuf))
        {
          return RECEIVED_FRAME_STATUS_FAIL;
        }

        AssociationGet(
            rxOpt->destNode.endpoint,
            (uint8_t *)&pCmd->ZW_MultiChannelAssociationGetV3Frame.cmdClass,
            (uint8_t *)pTxBuf,
            &outgoingFrameLength);

        RxToTxOptions(rxOpt, &pTxOptionsEx);

        // Transmit the stuff.
        txResult = Transport_SendResponseEP(
            (BYTE *)pTxBuf,
            outgoingFrameLength,
            pTxOptionsEx,
            ZCB_ResponseJobStatus);
        if (ZW_TX_IN_PROGRESS != txResult)
        {
          FreeResponseBuffer();
        }
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;

    case MULTI_CHANNEL_ASSOCIATION_SET_V2:
      handleAssociationSet(
          rxOpt->destNode.endpoint,
          (ZW_MULTI_CHANNEL_ASSOCIATION_SET_1BYTE_V2_FRAME*)pCmd,
          cmdLength);
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;

    case MULTI_CHANNEL_ASSOCIATION_REMOVE_V2:
      if (2 == cmdLength)
      {
        /*
         * According to the CC Multi Channel spec, the remove command MAY be interpreted with only
         * command class and command resulting in a command length of 2 bytes. This is interpreted as if
         * the command was [Class, Command, GroupID=0] which will remove all nodes in all groups.
         * Hence, set group ID to zero.
         */
        pCmd->ZW_MultiChannelAssociationRemove1byteV2Frame.groupingIdentifier = 0;
      }
      AssociationRemove(
          pCmd->ZW_MultiChannelAssociationRemove1byteV2Frame.groupingIdentifier,
          rxOpt->destNode.endpoint,
          (ZW_MULTI_CHANNEL_ASSOCIATION_REMOVE_1BYTE_V2_FRAME*)pCmd,
          cmdLength);
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;

    case MULTI_CHANNEL_ASSOCIATION_GROUPINGS_GET_V2:
      pTxBuf = GetResponseBuffer();
      /*Check pTxBuf is free*/
      if( NON_NULL( pTxBuf ) )
      {
        RxToTxOptions(rxOpt, &pTxOptionsEx);
        pTxBuf->ZW_MultiChannelAssociationGroupingsReportV2Frame.cmdClass = COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2;
        pTxBuf->ZW_MultiChannelAssociationGroupingsReportV2Frame.cmd = MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT_V2;
        pTxBuf->ZW_MultiChannelAssociationGroupingsReportV2Frame.supportedGroupings = handleGetMaxAssociationGroups(rxOpt->destNode.endpoint);
        if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP( (BYTE *)pTxBuf,
                                      sizeof(pTxBuf->ZW_MultiChannelAssociationGroupingsReportV2Frame),
                                      pTxOptionsEx,
                                      ZCB_ResponseJobStatus))
        {
          /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
          FreeResponseBuffer();
        }
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}
