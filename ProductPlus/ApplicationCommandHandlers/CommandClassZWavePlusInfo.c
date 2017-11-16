/***************************************************************************
*
* Copyright (c) 2001-2011
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: ZWave+ Info Command Class source file
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
#include <ZW_typedefs.h>
#include "config_app.h"
#include <CommandClassZWavePlusInfo.h>
#include <misc.h>
#include <ZW_uart_api.h>
#include <ZW_plus_version.h>
/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_ZWAVE_INFO
#define ZW_DEBUG_ZWAVE_INFO_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_ZWAVE_INFO_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_ZWAVE_INFO_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_ZWAVE_INFO_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_ZWAVE_INFO_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_ZWAVE_INFO_SEND_BYTE(data)
#define ZW_DEBUG_ZWAVE_INFO_SEND_STR(STR)
#define ZW_DEBUG_ZWAVE_INFO_SEND_NUM(data)
#define ZW_DEBUG_ZWAVE_INFO_SEND_WORD_NUM(data)
#define ZW_DEBUG_ZWAVE_INFO_SEND_NL()
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

/*==============================   handleCommandClassZWavePlusInfo  ============
**
**  Function:  handler for ZWave Plus Info CC
**
**  Side effects: None
**
**--------------------------------------------------------------------------*/
void
handleCommandClassZWavePlusInfo(
  BYTE  option,                 /* IN Frame header info */
  BYTE  sourceNode,               /* IN Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, the union */
  /*    should be used to access the fields */
  BYTE   cmdLength                /* IN Number of command bytes including the command */
)
{
  if (pCmd->ZW_Common.cmd == ZWAVEPLUS_INFO_GET)
  {
    ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
    /*Check pTxBuf is free*/
    if(NULL != pTxBuf)
    {
      pTxBuf->ZW_ZwaveplusInfoReportV2Frame.cmdClass = COMMAND_CLASS_ZWAVEPLUS_INFO;
      pTxBuf->ZW_ZwaveplusInfoReportV2Frame.cmd = ZWAVEPLUS_INFO_REPORT;
      pTxBuf->ZW_ZwaveplusInfoReportV2Frame.zWaveVersion = ZW_PLUS_VERSION;
      pTxBuf->ZW_ZwaveplusInfoReportV2Frame.roleType = APP_ROLE_TYPE;
      pTxBuf->ZW_ZwaveplusInfoReportV2Frame.nodeType = APP_NODE_TYPE;
      pTxBuf->ZW_ZwaveplusInfoReportV2Frame.installerIconType1 = (APP_ICON_TYPE >> 8);
      pTxBuf->ZW_ZwaveplusInfoReportV2Frame.installerIconType2 = (APP_ICON_TYPE & 0xff);
      pTxBuf->ZW_ZwaveplusInfoReportV2Frame.userIconType1 = (APP_USER_ICON_TYPE >> 8);
      pTxBuf->ZW_ZwaveplusInfoReportV2Frame.userIconType2 = (APP_USER_ICON_TYPE & 0xff);

      ZW_DEBUG_ZWAVE_INFO_SEND_NL();
      ZW_DEBUG_ZWAVE_INFO_SEND_STR("ZW+Info: ");
      ZW_DEBUG_ZWAVE_INFO_SEND_NUM(pTxBuf->ZW_ZwaveplusInfoReportV2Frame.installerIconType1);
      ZW_DEBUG_ZWAVE_INFO_SEND_NUM(pTxBuf->ZW_ZwaveplusInfoReportV2Frame.installerIconType2);
      ZW_DEBUG_ZWAVE_INFO_SEND_BYTE(' ');
      ZW_DEBUG_ZWAVE_INFO_SEND_NUM(pTxBuf->ZW_ZwaveplusInfoReportV2Frame.userIconType1);
      ZW_DEBUG_ZWAVE_INFO_SEND_NUM(pTxBuf->ZW_ZwaveplusInfoReportV2Frame.userIconType2);
      ZW_DEBUG_ZWAVE_INFO_SEND_NL();
      if(FALSE == Transport_SendResponse(
          sourceNode,
          (BYTE *)pTxBuf,
          sizeof(pTxBuf->ZW_ZwaveplusInfoReportV2Frame),
          option,
          ZCB_ResponseJobStatus))
      {
        /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
        FreeResponseBuffer();
      }
    }
  }
}
