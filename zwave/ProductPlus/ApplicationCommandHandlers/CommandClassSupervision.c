/**
 * @file CommandClassSupervision.c
 * @brief Supervision Command Class
 * @author Christian Salmony Olsen
 * @author Thomas Roll
 * @copyright Copyright (c) 2001-2016
 * Sigma Designs, Inc.
 * All Rights Reserved
 * @details None
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_stdint.h>
#include <ZW_TransportEndpoint.h>
#include "config_app.h"
#include <CommandClassSupervision.h>
#include <ZW_uart_api.h>
#include <ZW_TransportMulticast.h>
#include <misc.h>
#include <ZW_tx_mutex.h>
#include <ZW_mem_api.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_DEBUG_CC_SUPERVISION
#define ZW_DEBUG_CC_SUPERVISION_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_CC_SUPERVISION_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_CC_SUPERVISION_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_CC_SUPERVISION_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_CC_SUPERVISION_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_CC_SUPERVISION_SEND_BYTE(data)
#define ZW_DEBUG_CC_SUPERVISION_SEND_STR(STR)
#define ZW_DEBUG_CC_SUPERVISION_SEND_NUM(data)
#define ZW_DEBUG_CC_SUPERVISION_SEND_WORD_NUM(data)
#define ZW_DEBUG_CC_SUPERVISION_SEND_NL()
#endif

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

static BYTE m_sessionId = 1;
static BYTE m_CommandLength = 0;
static uint8_t previously_receive_session_id = 0;
static uint8_t previously_rxStatus = 0;

static cc_supervision_status_updates_t m_status_updates = CC_SUPERVISION_STATUS_UPDATES_NOT_SUPPORTED;
VOID_CALLBACKFUNC(m_pGetReceivedHandler)(SUPERVISION_GET_RECEIVED_HANDLER_ARGS * pArgs) = NULL;
VOID_CALLBACKFUNC(m_pReportReceivedHandler)(cc_supervision_status_t status, BYTE duration) = NULL;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

void
CommandClassSupervisionInit( cc_supervision_status_updates_t status_updates,
        VOID_CALLBACKFUNC(pGetReceivedHandler)(SUPERVISION_GET_RECEIVED_HANDLER_ARGS * pArgs),
        VOID_CALLBACKFUNC(pReportReceivedHandler)(cc_supervision_status_t status, BYTE duration))
{
  m_status_updates = status_updates;
  m_pGetReceivedHandler = pGetReceivedHandler;
  m_pReportReceivedHandler = pReportReceivedHandler;
  previously_receive_session_id = 0;
}

received_frame_status_t
handleCommandClassSupervision(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt, /* IN receive options of type RECEIVE_OPTIONS_TYPE_EX  */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN  Payload from the received frame */
  BYTE cmdLength)               /* IN Number of command bytes including the command */
{
  static MULTICHAN_DEST_NODE_ID previously_received_destination;

  UNUSED(cmdLength);
  UNUSED(rxOpt);

  switch (pCmd->ZW_Common.cmd)
  {
    case SUPERVISION_GET:
    {
      /**
       * SUPERVISION_GET handle:
       * 1. Single-cast:
       *    a. Transport_ApplicationCommandHandlerEx() is called by checking previously_receive_session_id.
       *    b. single-cast trigger supervision_report is send back.
       *
       * 2. Multi-cast:
       *    a. If multi-cast is received (rxStatus includes flag RECEIVE_STATUS_TYPE_MULTI).
       *    b. Transport_ApplicationCommandHandlerEx() is called
       *    c. Do not send supervision_report.
       *
       * 3. Multi-cast single-cast follow up:
       *    a. Transport_ApplicationCommandHandlerEx is discarded on single-cast by checking previously_receive_session_id.
       *    b. single-cast trigger supervision_report is send back.
       *
       * 4. Single-cast CC multichannel bit-adr.:
       *    CommandClassMultiChan handle bit addressing by calling each endpoint with the payload.
       *    a. If Single-cast CC multichannel bit-adr. (rxStatus includes flag RECEIVE_STATUS_TYPE_MULTI).
       *    b. Transport_ApplicationCommandHandlerEx() must be called every time. Check previously_received_destination
       *       differ from EXTRACT_SESSION_ID(pCmd->ZW_SupervisionGetFrame.sessionid)
       *    c. Do not send supervision_report.
       */
      uint8_t properties1;
      /*
       * status need to be static to handle multi-cast single-cast follow up.
       * Multi-cast get status Transport_ApplicationCommandHandlerEx() and sing-cast send Supervision report.
       */
      static cc_supervision_status_t status = CC_SUPERVISION_STATUS_NOT_SUPPORTED;
      uint8_t duration;
      TRANSMIT_OPTIONS_TYPE_SINGLE_EX * pTxOptions;

      ZW_DEBUG_CC_SUPERVISION_SEND_STR("\r\nCall CC handler\r\n");
      ActivateFlagSupervisionEncap();

      if(previously_receive_session_id != CC_SUPERVISION_EXTRACT_SESSION_ID(pCmd->ZW_SupervisionGetFrame.properties1))
      {
        /*
         * Reset status session id is changed.
         */
        status = CC_SUPERVISION_STATUS_NOT_SUPPORTED;
      }

      if ( previously_receive_session_id != CC_SUPERVISION_EXTRACT_SESSION_ID(pCmd->ZW_SupervisionGetFrame.properties1) ||   /*if previously session-id differ from current OR */
           (0 != memcmp((BYTE*)&rxOpt->destNode,                                                /*if previously [node-id + endpoint] differ from current THEN*/
                        (BYTE*)&previously_received_destination,                  /*PARSE frame to Transport_ApplicationCommandHandlerEx*/
                        sizeof(MULTICHAN_DEST_NODE_ID))) )
      {
        status = (received_frame_status_t)Transport_ApplicationCommandHandlerEx(
                rxOpt,
                (ZW_APPLICATION_TX_BUFFER *)(((BYTE *)pCmd) + sizeof(ZW_SUPERVISION_GET_FRAME)),
                (pCmd->ZW_SupervisionGetFrame.encapsulatedCommandLength));
      }


      /* RECEIVE_STATUS_TYPE_MULTI: It is a multi-cast frame or CC multichannel bit-adr frame.*/
      if (rxOpt->rxStatus & RECEIVE_STATUS_TYPE_MULTI)
      {
        ZW_DEBUG_CC_SUPERVISION_SEND_NL();
        ZW_DEBUG_CC_SUPERVISION_SEND_STR("Multicast");

        /*update previously session-id and reset previously_received_destination [node-id + endpoint]*/
        previously_receive_session_id = CC_SUPERVISION_EXTRACT_SESSION_ID(pCmd->ZW_SupervisionGetFrame.properties1);
        memcpy((BYTE*)&previously_received_destination, (BYTE*)&rxOpt->destNode, sizeof(MULTICHAN_DEST_NODE_ID));
        previously_rxStatus = RECEIVE_STATUS_TYPE_MULTI;

        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      else
      {
        /*CC:006C.01.01.11.009: A receiving node MUST ignore duplicate singlecast commands having the same Session ID*/
        if ( previously_receive_session_id == CC_SUPERVISION_EXTRACT_SESSION_ID(pCmd->ZW_SupervisionGetFrame.properties1) &&
             (0 == memcmp((BYTE*)&rxOpt->destNode,
                          (BYTE*)&previously_received_destination,
                          sizeof(MULTICHAN_DEST_NODE_ID))) &&
                          0 == previously_rxStatus )
        {
          return RECEIVED_FRAME_STATUS_FAIL;
        }
      }
      /*update previously session-id and reset previously_received_destination [node-id + endpoint]*/
      previously_receive_session_id = CC_SUPERVISION_EXTRACT_SESSION_ID(pCmd->ZW_SupervisionGetFrame.properties1);
      memcpy((BYTE*)&previously_received_destination, (BYTE*)&rxOpt->destNode, sizeof(MULTICHAN_DEST_NODE_ID));
      previously_rxStatus = 0;

      if (NON_NULL(m_pGetReceivedHandler))
      {
        // Call the assigned function.
        SUPERVISION_GET_RECEIVED_HANDLER_ARGS args;

        args.cmdClass = *(((BYTE *)pCmd) + sizeof(ZW_SUPERVISION_GET_FRAME));
        args.cmd      = *(((BYTE *)pCmd) + sizeof(ZW_SUPERVISION_GET_FRAME) + 1);
        args.properties1 = pCmd->ZW_SupervisionGetFrame.properties1;
        args.rxOpt = rxOpt;

        m_pGetReceivedHandler(&args);

        status = args.status;
        duration = args.duration;
        properties1 = args.properties1;
      }
      else
      {
        // Set variables to standard values.
        duration = 0;
        properties1 = CC_SUPERVISION_EXTRACT_SESSION_ID(pCmd->ZW_SupervisionGetFrame.properties1);
        properties1 |= CC_SUPERVISION_ADD_MORE_STATUS_UPDATE(CC_SUPERVISION_MORE_STATUS_UPDATES_THIS_IS_LAST);
      }


      // When we have gotten the information, we can send a Supervision report.
      RxToTxOptions(rxOpt, &pTxOptions);
      CmdClassSupervisionReportSend(
              pTxOptions,
              properties1,
              status,
              duration,
              NULL);
    }
    return RECEIVED_FRAME_STATUS_SUCCESS;
    break;

    case SUPERVISION_REPORT:
      if ((m_sessionId - 1) == pCmd->ZW_SupervisionReportFrame.properties1)
      {
        // The received session ID matches the one we sent.
        ZW_DEBUG_CC_SUPERVISION_SEND_NL();
        ZW_DEBUG_CC_SUPERVISION_SEND_STR("Session ID match");

        if (NON_NULL(m_pReportReceivedHandler))
        {
          m_pReportReceivedHandler(
                  pCmd->ZW_SupervisionReportFrame.status,
                  pCmd->ZW_SupervisionReportFrame.duration);
        }

        ZW_TransportMulticast_clearTimeout();

        ZW_DEBUG_CC_SUPERVISION_SEND_NL();
        ZW_DEBUG_CC_SUPERVISION_SEND_STR("Src: ");
        ZW_DEBUG_CC_SUPERVISION_SEND_NUM(rxOpt->sourceNode.nodeId);
      }
      else
      {
        ZW_DEBUG_CC_SUPERVISION_SEND_NL();
        ZW_DEBUG_CC_SUPERVISION_SEND_STR("Session ID NO match");
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}

JOB_STATUS
CmdClassSupervisionReportSend(
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX* pTxOptionsEx,
  uint8_t properties,
  cc_supervision_status_t status,
  uint8_t duration,
  VOID_CALLBACKFUNC(pCallback)(TRANSMISSION_RESULT * pTransmissionResult))
{
  ZW_APPLICATION_TX_BUFFER *pTxBuf = GetRequestBuffer(pCallback);
  if (IS_NULL( pTxBuf ))
  {
    /*Ongoing job is active.. just stop current job*/
    return JOB_STATUS_BUSY;
  }

  pTxBuf->ZW_SupervisionReportFrame.cmdClass = COMMAND_CLASS_SUPERVISION;
  pTxBuf->ZW_SupervisionReportFrame.cmd = SUPERVISION_REPORT;
  pTxBuf->ZW_SupervisionReportFrame.properties1 = properties;
  pTxBuf->ZW_SupervisionReportFrame.status = status;
  pTxBuf->ZW_SupervisionReportFrame.duration = duration;

  if(ZW_TX_IN_PROGRESS != Transport_SendRequestEP(
      (BYTE *)pTxBuf,
      sizeof(ZW_SUPERVISION_REPORT_FRAME),
      pTxOptionsEx,
      ZCB_RequestJobStatus))
  {
    /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
     FreeRequestBuffer();
     return JOB_STATUS_BUSY;
  }
  return JOB_STATUS_SUCCESS;
}

/**
 * CommandClassSupervisionGetAdd
 */
void CommandClassSupervisionGetAdd(ZW_SUPERVISION_GET_FRAME* pbuf)
{
  m_CommandLength = 0;
  CommandClassSupervisionGetWrite(pbuf);
  m_sessionId = ((m_sessionId + 1) > 0x3F) ? 1 : (m_sessionId + 1); /* increment m_sessionId, wrap around if over 0x3F */
}

/**
 * CommandClassSupervisionGetWrite
 */
void CommandClassSupervisionGetWrite(ZW_SUPERVISION_GET_FRAME* pbuf)
{
  pbuf->cmdClass =  COMMAND_CLASS_SUPERVISION;
  pbuf->cmd = SUPERVISION_GET;
  pbuf->properties1 = CC_SUPERVISION_ADD_SESSION_ID(m_sessionId);
  pbuf->properties1 |= CC_SUPERVISION_ADD_STATUS_UPDATE(m_status_updates);
  pbuf->encapsulatedCommandLength = m_CommandLength;
}

/**
 * CommandClassSupervisionGetSetPayloadLength
 */
void CommandClassSupervisionGetSetPayloadLength(ZW_SUPERVISION_GET_FRAME* pbuf, BYTE payLoadlen)
{
  pbuf->encapsulatedCommandLength = payLoadlen;
  m_CommandLength = payLoadlen;
}

/**
 * CommandClassSupervisionGetGetPayloadLength
 */
BYTE CommandClassSupervisionGetGetPayloadLength(ZW_SUPERVISION_GET_FRAME* pbuf)
{
  return pbuf->encapsulatedCommandLength;
}
