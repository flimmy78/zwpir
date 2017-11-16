/*************************************************************************** 
* 
* Copyright (c) 2001-2011 
* Sigma Designs, Inc. 
* All Rights Reserved 
* 
*--------------------------------------------------------------------------- 
* 
* Description: Basic Command Class source file
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
#include <CommandClassBasic.h>
#include "misc.h"

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

/*==============================   handleCommandClassBasic  ============
**
**  Function:  handler for Basic CC
**
**  Side effects: None
**
**--------------------------------------------------------------------------*/
void 
handleCommandClassBasic(
  BYTE  option,                 /* IN Frame header info */
  BYTE  sourceNode,               /* IN Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, the union */
  /*    should be used to access the fields */
  BYTE   cmdLength                /* IN Number of command bytes including the command */
  
)
{
  switch (pCmd->ZW_Common.cmd)
  {
      //Must be ignored to avoid unintentional operation. Cannot be mapped to another command class.
    case BASIC_SET:
      handleBasicSetCommand(pCmd->ZW_BasicSetFrame.value);
      break;

    case BASIC_GET:
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if(NULL != pTxBuf)
        {
          /* Controller wants the sensor level */
          pTxBuf->ZW_BasicReportFrame.cmdClass = COMMAND_CLASS_BASIC;
          pTxBuf->ZW_BasicReportFrame.cmd = BASIC_REPORT;
    
          pTxBuf->ZW_BasicReportFrame.value =  getAppBasicReport();
          if(FALSE == Transport_SendResponse(
              sourceNode,
              (BYTE *)pTxBuf,
              sizeof(ZW_BASIC_REPORT_FRAME),
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


/*================= CmdClassBasicSetSend =======================
** Function description
** This function...
**
** Side effects: 
**
**-------------------------------------------------------------------------*/
JOB_STATUS
CmdClassBasicSetSend(
  BYTE option,
  BYTE dstNode,
  BYTE bValue,
  VOID_CALLBACKFUNC(pCbFunc)(BYTE val))
{
  ZW_APPLICATION_TX_BUFFER *pTxBuf = GetRequestBuffer(pCbFunc);
  if(NULL == pTxBuf)
  {
    /*Ongoing job is active.. just stop current job*/
    return JOB_STATUS_BUSY;
  }
 
  pTxBuf->ZW_BasicSetFrame.cmdClass = COMMAND_CLASS_BASIC;
  pTxBuf->ZW_BasicSetFrame.cmd = BASIC_SET;
  pTxBuf->ZW_BasicSetFrame.value =  bValue;

  if(FALSE == Transport_SendRequest(
      dstNode,
      (BYTE *)pTxBuf,
      sizeof(ZW_BASIC_SET_FRAME),
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

/*================= CmdClassBasicReportSend =======================
** Function description
** This function...
**
** Side effects: 
**
**-------------------------------------------------------------------------*/
JOB_STATUS
CmdClassBasicReportSend(
  BYTE option,
  BYTE dstNode,
  BYTE bValue,
  VOID_CALLBACKFUNC(pCbFunc)(BYTE val))
{
  ZW_APPLICATION_TX_BUFFER *pTxBuf = GetRequestBuffer(pCbFunc);
  if(NULL == pTxBuf)
  {
    /*Ongoing job is active.. just stop current job*/
    return JOB_STATUS_BUSY;
  }
 
  pTxBuf->ZW_BasicReportFrame.cmdClass = COMMAND_CLASS_BASIC;
  pTxBuf->ZW_BasicReportFrame.cmd = BASIC_REPORT;
  pTxBuf->ZW_BasicReportFrame.value =  bValue;

  if(FALSE == Transport_SendRequest(
      dstNode,
      (BYTE *)pTxBuf,
      sizeof(ZW_BASIC_REPORT_FRAME),
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
