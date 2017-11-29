/***************************************************************************
*
* Copyright (c) 2001-2011
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Version Command Class source file
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
#include <ZW_TransportLayer.h>
#include <ZW_tx_mutex.h>
#include <ZW_nvr_app_api.h>
#include "config_app.h"
#include <CommandClassVersion.h>
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

received_frame_status_t
handleCommandClassVersion(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength)
{
  PROTOCOL_VERSION protocolVersion;
  UNUSED(cmdLength);

  if(TRUE == Check_not_legal_response_job(rxOpt))
  {
    /*Do not support endpoint bit-addressing */
    return RECEIVED_FRAME_STATUS_FAIL;
  }

  switch (pCmd->ZW_VersionGetFrame.cmd)
  {
    case VERSION_GET_V2:
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if( NON_NULL( pTxBuf ) )
        {
          BYTE n;/*firmware target number 1..N */
          TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
          RxToTxOptions(rxOpt, &pTxOptionsEx);
          pTxBuf->ZW_VersionReport1byteV2Frame.cmdClass = COMMAND_CLASS_VERSION_V2;
          pTxBuf->ZW_VersionReport1byteV2Frame.cmd = VERSION_REPORT_V2;
          pTxBuf->ZW_VersionReport1byteV2Frame.zWaveLibraryType = ZW_TYPE_LIBRARY();
          ZW_GetProtocolVersion(&protocolVersion);
          pTxBuf->ZW_VersionReport1byteV2Frame.zWaveProtocolVersion = protocolVersion.protocolVersionMajor;
          pTxBuf->ZW_VersionReport1byteV2Frame.zWaveProtocolSubVersion = protocolVersion.protocolVersionMinor;
          handleGetFirmwareVersion( 0,
            (VG_VERSION_REPORT_V2_VG*)&(pTxBuf->ZW_VersionReport1byteV2Frame.firmware0Version));
          pTxBuf->ZW_VersionReport1byteV2Frame.hardwareVersion = handleGetFirmwareHwVersion();
          pTxBuf->ZW_VersionReport1byteV2Frame.numberOfFirmwareTargets = handleNbrFirmwareVersions() - 1;/*-1 : Firmware version 0*/

          for( n = 1; n < handleNbrFirmwareVersions();n++)
          {
#ifdef __C51__
            handleGetFirmwareVersion( n, &(pTxBuf->ZW_VersionReport1byteV2Frame.variantgroup1[n-1]));
#endif
          }

          if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
              (BYTE *)pTxBuf,
              /* comment to len calc: frame size           + size of number of firmwareversions                                 -   variantgroup1 (calc in size of number of firmwareversions)*/
              sizeof(pTxBuf->ZW_VersionReport1byteV2Frame) + (handleNbrFirmwareVersions() - 1)* sizeof(VG_VERSION_REPORT_V2_VG) - sizeof(VG_VERSION_REPORT_V2_VG) , /*-1 is Firmware version 0*/
              pTxOptionsEx,
              ZCB_ResponseJobStatus))
          {
            /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
            FreeResponseBuffer();
          }
          return RECEIVED_FRAME_STATUS_SUCCESS;
        }
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;

    case VERSION_COMMAND_CLASS_GET_V2:
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if( NON_NULL( pTxBuf ) )
        {
          TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
          RxToTxOptions(rxOpt, &pTxOptionsEx);
          pTxBuf->ZW_VersionCommandClassReportFrame.cmdClass = COMMAND_CLASS_VERSION_V2;
          pTxBuf->ZW_VersionCommandClassReportFrame.cmd = VERSION_COMMAND_CLASS_REPORT_V2;
          pTxBuf->ZW_VersionCommandClassReportFrame.requestedCommandClass = pCmd->ZW_VersionCommandClassGetFrame.requestedCommandClass;
          pTxBuf->ZW_VersionCommandClassReportFrame.commandClassVersion = handleCommandClassVersionAppl(pCmd->ZW_VersionCommandClassGetFrame.requestedCommandClass);

          if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
              (BYTE *)pTxBuf,
              sizeof(pTxBuf->ZW_VersionCommandClassReportFrame),
              pTxOptionsEx,
              ZCB_ResponseJobStatus))
          {
            /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
            FreeResponseBuffer();
          }
          return RECEIVED_FRAME_STATUS_SUCCESS;
        }
      }
      return RECEIVED_FRAME_STATUS_FAIL;
      break;

    default:
      break;
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}

BYTE
handleGetFirmwareHwVersion(void)
{
  BYTE hwVersion;
  /* Read hwVersion from NVR.*/
  ZW_NVRGetAppValue(offsetof(NVR_APP_FLASH_STRUCT, hwVersion), 1, &hwVersion );
  return hwVersion; /*HW version*/
}
