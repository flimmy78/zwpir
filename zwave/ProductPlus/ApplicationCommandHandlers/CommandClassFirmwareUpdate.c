/***************************************************************************
*
* Copyright (c) 2001-2013
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Some nice descriptive description.
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2013/07/04 13:51:13 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <ZW_TransportLayer.h>
#include <misc.h>
#include "config_app.h"
#include <ota_util.h>
#include <event_util.h>
#include <ZW_tx_mutex.h>
#include <ZW_uart_api.h>
#include <ZW_crc.h>
#include <ZW_firmware_descriptor.h>
#include <ZW_Firmware_bootloader_defs.h>
#include <ZW_firmware_update_nvm_api.h>
#include <CommandClassFirmwareUpdate.h>


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



/*============================ handleCommandClassFWUpdate ===============================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
handleCommandClassFWUpdate(
  BYTE  option,                 /* IN Frame header info */
  BYTE  sourceNode,               /* IN Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, the union */
  /*    should be used to access the fields */
  BYTE   cmdLength                /* IN Number of command bytes including the command */
)
{
  switch (pCmd->ZW_Common.cmd)
  {
    case FIRMWARE_MD_GET_V2:
    {
      ZW_APPLICATION_TX_BUFFER* pTxBuf = GetResponseBuffer();
      if(NULL != pTxBuf)
      {
        pTxBuf->ZW_FirmwareMdReportV2Frame.cmdClass = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2;
        pTxBuf->ZW_FirmwareMdReportV2Frame.cmd = FIRMWARE_MD_REPORT_V2;
        pTxBuf->ZW_FirmwareMdReportV2Frame.manufacturerId1 = firmwareDescriptor.manufacturerID >> 8;
        pTxBuf->ZW_FirmwareMdReportV2Frame.manufacturerId2 = firmwareDescriptor.manufacturerID & 0xFF;
        pTxBuf->ZW_FirmwareMdReportV2Frame.firmwareId1 = firmwareDescriptor.firmwareID >> 8;
        pTxBuf->ZW_FirmwareMdReportV2Frame.firmwareId2 = firmwareDescriptor.firmwareID & 0xFF;
        pTxBuf->ZW_FirmwareMdReportV2Frame.checksum1 = firmwareDescriptor.checksum >> 8;
        pTxBuf->ZW_FirmwareMdReportV2Frame.checksum2 = firmwareDescriptor.checksum & 0xFF;

        if ( FALSE == Transport_SendResponse(sourceNode,
               (BYTE *)pTxBuf,
               sizeof(ZW_FIRMWARE_MD_REPORT_V2_FRAME),
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
    case FIRMWARE_UPDATE_MD_REPORT_V2:
      {
        WORD crc16Result = ZW_CheckCrc16(0x1D0F, &(pCmd->ZW_Common.cmdClass), cmdLength);
        WORD  firmwareUpdateReportNumber = ((WORD)(pCmd->ZW_FirmwareUpdateMdReport1byteV2Frame.properties1 &
                                      FIRMWARE_UPDATE_MD_REPORT_PROPERTIES1_REPORT_NUMBER_1_MASK_V2) << 8) +
                                     (WORD)(pCmd->ZW_FirmwareUpdateMdReport1byteV2Frame.reportNumber2);
        BYTE fw_actualFrameSize =  cmdLength -
                                  /* Calculate length of actual data1 field */
                                  (sizeof(pCmd->ZW_FirmwareUpdateMdReport1byteV2Frame.cmdClass) +
                                   sizeof(pCmd->ZW_FirmwareUpdateMdReport1byteV2Frame.cmd) +
                                   sizeof(pCmd->ZW_FirmwareUpdateMdReport1byteV2Frame.properties1) +
                                   sizeof(pCmd->ZW_FirmwareUpdateMdReport1byteV2Frame.reportNumber2) +
                                   sizeof(pCmd->ZW_FirmwareUpdateMdReport1byteV2Frame.checksum1) +
                                   sizeof(pCmd->ZW_FirmwareUpdateMdReport1byteV2Frame.checksum2));


        handleCmdClassFirmwareUpdateMdReport(crc16Result,
                                             firmwareUpdateReportNumber,
                                             pCmd->ZW_FirmwareUpdateMdReport1byteV2Frame.properties1,
                                             &(pCmd->ZW_FirmwareUpdateMdReport1byteV2Frame.data1),
                                             fw_actualFrameSize);


      }
      break;
    case FIRMWARE_UPDATE_MD_REQUEST_GET_V2:
      {
        ZW_APPLICATION_TX_BUFFER* pTxBuf = GetResponseBufferCb(ZCB_CmdClassFwUpdateMdReqReport);
        /*Check pTxBuf is free*/
        if(NULL != pTxBuf)
        {
          pTxBuf->ZW_FirmwareUpdateMdRequestReportV2Frame.cmdClass = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2;
          pTxBuf->ZW_FirmwareUpdateMdRequestReportV2Frame.cmd = FIRMWARE_UPDATE_MD_REQUEST_REPORT_V2;
          handleCmdClassFirmwareUpdateMdReqGet( sourceNode,
            (FW_UPDATE_GET*) &(pCmd->ZW_FirmwareUpdateMdRequestGetV2Frame.manufacturerId1),
            &(pTxBuf->ZW_FirmwareUpdateMdRequestReportV2Frame.status));

          if (!Transport_SendResponse(
                  sourceNode,
                  (BYTE*)pTxBuf,
                  sizeof(ZW_FIRMWARE_UPDATE_MD_REQUEST_REPORT_V2_FRAME),
                  option,
                  ZCB_ResponseJobStatus))
          {
            FreeResponseBuffer();
            ZCB_CmdClassFwUpdateMdReqReport(TRANSMIT_COMPLETE_FAIL);
          }
        }
      }
      break;
    default:
      break;
  }
}

/*============================ CmdClassFirmwareUpdateMdStatusReport ===============================
** Function description
** This function...
** Values used for Firmware Update Md Status Report command
** FIRMWARE_UPDATE_MD_STATUS_REPORT_UNABLE_TO_RECEIVE_WITHOUT_CHECKSUM_ERROR_V2     0x00
** FIRMWARE_UPDATE_MD_STATUS_REPORT_UNABLE_TO_RECEIVE_V2                            0x01
** FIRMWARE_UPDATE_MD_STATUS_REPORT_SUCCESSFULLY_V2                                 0xFF
**
** Side effects:
**
**-------------------------------------------------------------------------*/
JOB_STATUS
CmdClassFirmwareUpdateMdStatusReport(BYTE destNode, BYTE status, BYTE txOption, VOID_CALLBACKFUNC(pCbFunc)(BYTE val))
{
  ZW_APPLICATION_TX_BUFFER* pTxBuf = GetRequestBuffer(pCbFunc);
  /* Send status, when finished */
  if (pTxBuf != NULL)
  {
    WORD fw_crc;
    BYTE retVal;

    retVal = (TRUE == ZW_FirmwareUpdate_NVM_isValidCRC16(&fw_crc)) ? 1 : 0;
    pTxBuf->ZW_FirmwareUpdateMdStatusReportV2Frame.cmdClass = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2;
    pTxBuf->ZW_FirmwareUpdateMdStatusReportV2Frame.cmd = FIRMWARE_UPDATE_MD_STATUS_REPORT_V2;
    pTxBuf->ZW_FirmwareUpdateMdStatusReportV2Frame.status = status;
    /* Mark NVM Image Valid/Not Valid according to retVal */
    ZW_FirmwareUpdate_NVM_Set_NEWIMAGE((FIRMWARE_UPDATE_MD_STATUS_REPORT_SUCCESSFULLY_V2 == status) ? retVal : 0);
    if (!Transport_SendRequest(
          destNode,
          (BYTE*)pTxBuf,
          sizeof(ZW_FIRMWARE_UPDATE_MD_STATUS_REPORT_V2_FRAME),
          txOption,
          ZCB_RequestJobStatus, FALSE))
    {
      pCbFunc(0xFF);
      FreeRequestBuffer();
      return JOB_STATUS_BUSY;
    }
     return JOB_STATUS_SUCCESS;
  }
  return JOB_STATUS_BUSY;
}



/*============================ CmdClassFirmwareUpdateMdGet ==================
** Function description
** Send command Firmware update  MD Get
**
** Side effects:
**
**-------------------------------------------------------------------------*/
JOB_STATUS
CmdClassFirmwareUpdateMdGet( BYTE destNode, WORD firmwareUpdateReportNumber, BYTE txOption)
{
  ZW_APPLICATION_TX_BUFFER* pTxBuf = GetRequestBuffer(NULL);
  ZW_DEBUG_SEND_STR("CmdClassFirmwareUpdateMdGet");
  ZW_DEBUG_SEND_WORD_NUM(firmwareUpdateReportNumber);
  if (pTxBuf != NULL)
  {
  /* Ask for the next report */
    pTxBuf->ZW_FirmwareUpdateMdGetV2Frame.cmdClass = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2;
    pTxBuf->ZW_FirmwareUpdateMdGetV2Frame.cmd = FIRMWARE_UPDATE_MD_GET_V2;
    pTxBuf->ZW_FirmwareUpdateMdGetV2Frame.numberOfReports = 1;
    pTxBuf->ZW_FirmwareUpdateMdGetV2Frame.properties1 = firmwareUpdateReportNumber >> 8;
    pTxBuf->ZW_FirmwareUpdateMdGetV2Frame.reportNumber2 = firmwareUpdateReportNumber & 0xFF;
    if (!Transport_SendRequest(
           destNode,
           (BYTE*)pTxBuf,
           sizeof(ZW_FIRMWARE_UPDATE_MD_GET_V2_FRAME),
           txOption,
           ZCB_RequestJobStatus, FALSE))
    {
      FreeRequestBuffer();
      return JOB_STATUS_BUSY;
    }
    return JOB_STATUS_SUCCESS;
  }
  return JOB_STATUS_BUSY;
}
