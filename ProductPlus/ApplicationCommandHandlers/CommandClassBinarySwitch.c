/***************************************************************************
*
* Copyright (c) 2001-2011
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Binary switch Command Class cource file
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

#include "config_app.h"
#include <CommandClassBinarySwitch.h>
#include <misc.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

/*==============================   handleCommandClassBinarySwitch  ============
**
**  Function:  handler for Binary Switch Info CC
**
**  Side effects: None
**
**--------------------------------------------------------------------------*/
void
handleCommandClassBinarySwitch(
  BYTE  option,                 /* IN Frame header info */
  BYTE  sourceNode,               /* IN Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, the union */
  /*    should be used to access the fields */
  BYTE   cmdLength                /* IN Number of command bytes including the command */
)
{
  switch (pCmd->ZW_Common.cmd)
  {
    case SWITCH_BINARY_GET:
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if(NULL != pTxBuf)
        {
          pTxBuf->ZW_SwitchBinaryReportFrame.cmdClass = COMMAND_CLASS_SWITCH_BINARY;
          pTxBuf->ZW_SwitchBinaryReportFrame.cmd = SWITCH_BINARY_REPORT;
          pTxBuf->ZW_SwitchBinaryReportFrame.value = handleAppltBinarySwitchGet();
          if(FALSE == Transport_SendResponse(
              sourceNode,
              (BYTE *)pTxBuf,
              sizeof(ZW_SWITCH_BINARY_REPORT_FRAME),
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

    case SWITCH_BINARY_SET:
      ZCB_CmdCBinarySwitchSupportSet(pCmd->ZW_SwitchBinarySetFrame.switchValue);
      break;

    default:
      break;
  }
}

code const void (code * ZCB_CmdCBinarySwitchSupportSet_p)(BYTE txStatus) = &ZCB_CmdCBinarySwitchSupportSet;
/*============================ ZCB_CmdClassBinarySwitchSupportSet ===========
** Function description
** Check value is correct for current class and call application Set function.
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
ZCB_CmdCBinarySwitchSupportSet( BYTE val)
{
  if (val == 0)
  {
    handleApplBinarySwitchSet(CMD_CLASS_BIN_OFF);
  }

  else if ((val < 0x64) ||
           (val == 0xff))
  {
    handleApplBinarySwitchSet(CMD_CLASS_BIN_ON);
  }
}

/*============================ CmdClassBinarySwitchReportSendUnsolicited ====
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
JOB_STATUS
CmdClassBinarySwitchReportSendUnsolicited(
  BYTE option,
  BYTE dstNode,
  CMD_CLASS_BIN_SW_VAL bValue,
  VOID_CALLBACKFUNC(pCbFunc)(BYTE val))
{

  ZW_APPLICATION_TX_BUFFER *pTxBuf = GetRequestBuffer(pCbFunc);
  if(NULL == pTxBuf)
  {
    /*Ongoing job is active.. just stop current job*/
    return JOB_STATUS_BUSY;
  }

  pTxBuf->ZW_BasicReportFrame.cmdClass = COMMAND_CLASS_SWITCH_BINARY;
  pTxBuf->ZW_BasicReportFrame.cmd = SWITCH_BINARY_REPORT;
  pTxBuf->ZW_BasicReportFrame.value =  bValue;

  if(FALSE == Transport_SendRequest(
      dstNode,
      (BYTE *)pTxBuf,
      sizeof(ZW_SWITCH_BINARY_REPORT_FRAME),
      option,
      ZCB_RequestJobStatus,
      FALSE))
  {
    /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
     FreeRequestBuffer();
     return JOB_STATUS_BUSY;
  }
  return JOB_STATUS_SUCCESS;
}
