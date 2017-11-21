/***************************************************************************
*
* Copyright (c) 2001-2013
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: User Code Command Class
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2013/05/09 12:31:33 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_stdint.h>
#include <ZW_basis_api.h>
#include <ZW_tx_mutex.h>
#include <ZW_TransportLayer.h>

#include "config_app.h"
#include <CommandClassUserCode.h>
#include <misc.h>


/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_USERCODE
#include <ZW_uart_api.h>
#define ZW_DEBUG_USERCODE_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_USERCODE_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_USERCODE_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_USERCODE_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_USERCODE_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_USERCODE_SEND_BYTE(data)
#define ZW_DEBUG_USERCODE_SEND_STR(STR)
#define ZW_DEBUG_USERCODE_SEND_NUM(data)
#define ZW_DEBUG_USERCODE_SEND_WORD_NUM(data)
#define ZW_DEBUG_USERCODE_SEND_NL()
#endif

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/



/*============================ handleCommandClassUserCode ===================
** Function description
** Handle incoming command class User Code frames version 1.
**
** Side effects: none
**
**-------------------------------------------------------------------------*/
void
handleCommandClassUserCode(
  BYTE  option,
  BYTE  sourceNode,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE   cmdLength)
{
  switch (pCmd->ZW_Common.cmd)
  {
    case USER_CODE_GET:
      if( pCmd->ZW_UserCodeGetFrame.userIdentifier > 0 )
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if(NULL != pTxBuf)
        {
          BYTE len;
          pTxBuf->ZW_UserCodeReportFrame.cmdClass = COMMAND_CLASS_USER_CODE;
          pTxBuf->ZW_UserCodeReportFrame.cmd = USER_CODE_REPORT;
          pTxBuf->ZW_UserCodeReportFrame.userIdentifier = pCmd->ZW_UserCodeGetFrame.userIdentifier;
          handleCommandClassUserCodeIdGet( pCmd->ZW_UserCodeGetFrame.userIdentifier,
                                           (USER_ID_STATUS *)&(pTxBuf->ZW_UserCodeReportFrame.userIdStatus));
          if(FALSE == handleCommandClassUserCodeReport( pCmd->ZW_UserCodeGetFrame.userIdentifier,
                                            &(pTxBuf->ZW_UserCodeReportFrame.userCode1),
                                            &len))
          {
            /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
            FreeResponseBuffer();
            return; /*failing*/
          }
          if(! Transport_SendResponse(
                      sourceNode,
                      pTxBuf,
                      sizeof(ZW_USER_CODE_REPORT_FRAME) + len - USERCODE_MAX_LEN, /*TO#04176 return dynamic length*/
                      option,
                      ZCB_ResponseJobStatus))
          {
            /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
            FreeResponseBuffer();
          }
        }
        else
        {
          /*pTxBuf is occupied.. do nothing*/
        }
      }
      break;

    case USER_CODE_SET:
    {
      uint8_t userCodeLength = cmdLength - 4; // Frame length without user code is 4.
      uint8_t * pUserCodeChar = &(pCmd->ZW_UserCodeSetFrame.userCode1);
      uint8_t charCount;
      uint8_t userIdStatus = pCmd->ZW_UserCodeSetFrame.userIdStatus;

      if (userCodeLength < USERCODE_MIN_LEN || userCodeLength > USERCODE_MAX_LEN)
      {
        ZW_DEBUG_USERCODE_SEND_BYTE('a');
        return;
      }

      for (charCount = 0; charCount < userCodeLength; charCount++)
      {
        if (!(((*(pUserCodeChar + charCount) >= 0x30) && (*(pUserCodeChar + charCount) <= 0x39)) ||
            ((0 == userIdStatus) && (0 == *(pUserCodeChar + charCount)))))
        {
          ZW_DEBUG_USERCODE_SEND_BYTE('b');
          return;
        }
      }

      /*
      if ((userIdStatus > USER_ID_RESERVED) &&
          (userIdStatus != USER_ID_NO_STATUS))
          */
      if ((userIdStatus > USER_ID_RESERVED))
      {
        ZW_DEBUG_USERCODE_SEND_BYTE('c');
        return;
      }

      ZW_DEBUG_USERCODE_SEND_BYTE('d');

      handleCommandClassUserCodeSet(pCmd->ZW_UserCodeSetFrame.userIdentifier,
                                    userIdStatus,
                                    &(pCmd->ZW_UserCodeSetFrame.userCode1),
                                    userCodeLength);
    }
      break;

    case USERS_NUMBER_GET:
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if(NULL != pTxBuf)
        {
          pTxBuf->ZW_UsersNumberReportFrame.cmdClass = COMMAND_CLASS_USER_CODE;
          pTxBuf->ZW_UsersNumberReportFrame.cmd = USERS_NUMBER_REPORT;
          pTxBuf->ZW_UsersNumberReportFrame.supportedUsers = handleCommandClassUserCodeUsersNumberReport();

          if(FALSE == Transport_SendResponse(
                        sourceNode,
                        pTxBuf,
                        sizeof(ZW_USERS_NUMBER_REPORT_FRAME),
                        option,
                        ZCB_ResponseJobStatus))
          {
            /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
            FreeResponseBuffer();
          }
        }
        else
        {
          /*pTxBuf is occupied.. do nothing*/
        }
      }
      break;
  }
}


/*============================ CmdClassUserCodeSupportReport ===============
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
JOB_STATUS
CmdClassUserCodeSupportReport(
  BYTE destNode,
  BYTE userIdentifier,
  BYTE userIdStatus,
  BYTE* pUserCode,
  BYTE userCodeLen,
  VOID_CALLBACKFUNC(pCbFunc)(BYTE val))
{
  if( userIdentifier > 0 )
  {
    ZW_APPLICATION_TX_BUFFER *pTxBuf = GetRequestBuffer(pCbFunc);

    if(NULL == pTxBuf)
    {
      /*Ongoing job is active.. just stop current job*/
      return JOB_STATUS_BUSY;
    }

    pTxBuf->ZW_UserCodeReportFrame.cmdClass = COMMAND_CLASS_USER_CODE;
    pTxBuf->ZW_UserCodeReportFrame.cmd = USER_CODE_REPORT;
    pTxBuf->ZW_UserCodeReportFrame.userIdentifier = userIdentifier;
    pTxBuf->ZW_UserCodeReportFrame.userIdStatus = userIdStatus;
    if(NULL != pUserCode)
    {
      if(userCodeLen > USERCODE_MAX_LEN)
      {
        userCodeLen = USERCODE_MAX_LEN;
      }
      memcpy(&(pTxBuf->ZW_UserCodeReportFrame.userCode1), pUserCode, userCodeLen);
    }

    if(! Transport_SendRequest(
                destNode,
                pTxBuf,
                sizeof(ZW_USER_CODE_REPORT_FRAME) + userCodeLen - USERCODE_MAX_LEN, /*TO#04176 return dynamic length*/
                ZWAVE_PLUS_TX_OPTIONS,
                ZCB_RequestJobStatus, FALSE))
    {
      /*Free transmit-buffer mutex*/
      FreeRequestBuffer();
      return JOB_STATUS_BUSY;
    }
  }
  return JOB_STATUS_SUCCESS;
}
