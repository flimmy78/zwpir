#include "config_app.h"
#include <ZW_basis_api.h>
#include <ZW_TransportLayer.h>
#include <CommandClassSimpleAv.h>
#include <ZW_tx_mutex.h>

static BYTE seqNo;

/*============================ handleCommandClassSimpleAv ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
handleCommandClassSimpleAv(
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
    case SIMPLE_AV_CONTROL_GET:
    {
      ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
      if(NULL != pTxBuf)
      {
        /* Controller wants the sensor level */
        pTxBuf->ZW_SimpleAvControlReportFrame.cmdClass = COMMAND_CLASS_SIMPLE_AV_CONTROL;
        pTxBuf->ZW_SimpleAvControlReportFrame.cmd = SIMPLE_AV_CONTROL_REPORT;
        pTxBuf->ZW_SimpleAvControlReportFrame.numberOfReports =  getApplSimpleAvReports();
        if(FALSE == Transport_SendResponse(
            sourceNode,
            (BYTE *)pTxBuf,
            sizeof(ZW_SIMPLE_AV_CONTROL_GET_FRAME),
            option,
            ZCB_ResponseJobStatus))
        {
            /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
          FreeResponseBuffer();
        }
      }
    }
      break;

    case SIMPLE_AV_CONTROL_SUPPORTED_GET:
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if(NULL != pTxBuf)
        {
          /* Controller wants the sensor level */
          BYTE len = getApplSimpleAvSupported(pCmd->ZW_SimpleAvControlSupportedReport4byteFrame.reportNo,
                                        &pTxBuf->ZW_SimpleAvControlSupportedReport1byteFrame.bitMask1);
          pTxBuf->ZW_SimpleAvControlSupportedReport1byteFrame.cmdClass = COMMAND_CLASS_SIMPLE_AV_CONTROL;
          pTxBuf->ZW_SimpleAvControlSupportedReport1byteFrame.cmd = SIMPLE_AV_CONTROL_SUPPORTED_REPORT;
          pTxBuf->ZW_SimpleAvControlSupportedReport1byteFrame.reportNo = pCmd->ZW_SimpleAvControlSupportedReport4byteFrame.reportNo;
          if(FALSE == Transport_SendResponse(
              sourceNode,
              (BYTE *)pTxBuf,
              sizeof(ZW_SIMPLE_AV_CONTROL_SUPPORTED_REPORT_1BYTE_FRAME) - 1 +len,
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


/*================= CmdClassSimpleAvSet =======================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
JOB_STATUS
CmdClassSimpleAvSet(
  BYTE option,
  BYTE dstNode,
  WORD bCommand,                  /* IN What to do*/
  BYTE bKeyAttrib,                /*Key attribute*/
  VOID_CALLBACKFUNC(pCbFunc)(BYTE val))
{

  ZW_APPLICATION_TX_BUFFER *pTxBuf = GetRequestBuffer(pCbFunc);
  if(NULL == pTxBuf)
  {
    /*Ongoing job is active.. just stop current job*/
    return JOB_STATUS_BUSY;
  }
  pTxBuf->ZW_SimpleAvControlSet1byteFrame.cmdClass = COMMAND_CLASS_SIMPLE_AV_CONTROL;
  pTxBuf->ZW_SimpleAvControlSet1byteFrame.cmd = SIMPLE_AV_CONTROL_SET;
  pTxBuf->ZW_SimpleAvControlSet1byteFrame.sequenceNumber = seqNo++;
  pTxBuf->ZW_SimpleAvControlSet1byteFrame.properties1 = 0x07 & bKeyAttrib;
  pTxBuf->ZW_SimpleAvControlSet1byteFrame.itemId1 = 0;
  pTxBuf->ZW_SimpleAvControlSet1byteFrame.itemId2 = 0;
  pTxBuf->ZW_SimpleAvControlSet1byteFrame.variantgroup1.command1 = (BYTE)((bCommand&0xff00)>>8);   //Command MSB
  pTxBuf->ZW_SimpleAvControlSet1byteFrame.variantgroup1.command2 = (BYTE)(bCommand&0xff);        //Command LSB


  if(! Transport_SendRequest(
        dstNode,
        (BYTE*)pTxBuf,
        sizeof(ZW_SIMPLE_AV_CONTROL_SET_1BYTE_FRAME),
        option,
        ZCB_RequestJobStatus, FALSE))
  {
    FreeRequestBuffer();
    return JOB_STATUS_BUSY;
  }
  return JOB_STATUS_SUCCESS;
}
