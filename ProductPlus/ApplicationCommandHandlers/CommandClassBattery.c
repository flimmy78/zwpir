/***************************************************************************
*
* Copyright (c) 2001-2011
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Battery Command Class source file
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
#include <CommandClassBattery.h>
#include <ZW_adcdriv_api.h>
#include <ZW_timer_api.h>
#include "misc.h"

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#define UNS_BATT_TIMEOUT   30

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/


/*==============================   handleCommandClassBattery  ============
**
**  Function:  handler for Battery CC
**
**  Side effects: None
**
**--------------------------------------------------------------------------*/
void
handleCommandClassBattery(
  BYTE  option,                   /* IN Frame header info */
  BYTE  sourceNode,               /* IN Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, the union */
                                  /* should be used to access the fields */
  BYTE   cmdLength                /* IN Number of command bytes including the command */
)
{
  if (pCmd->ZW_Common.cmd == BATTERY_GET)
  {
    ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
    /*Check pTxBuf is free*/
    if(NULL != pTxBuf)
    {
      pTxBuf->ZW_BatteryReportFrame.cmdClass = COMMAND_CLASS_BATTERY;
      pTxBuf->ZW_BatteryReportFrame.cmd = BATTERY_REPORT;
      BatterySensorRead(&(pTxBuf->ZW_BatteryReportFrame.batteryLevel));
      if(FALSE == Transport_SendResponse(
          sourceNode,
          (BYTE *)pTxBuf,
          sizeof(ZW_BATTERY_REPORT_FRAME),
          option,
          ZCB_ResponseJobStatus))
      {
        /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
        FreeResponseBuffer();
      }
    }
  }
}

/*================= CmdClassBatteryReport =======================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
JOB_STATUS
CmdClassBatteryReport(
  BYTE option,
  BYTE dstNode,
  BYTE bBattLevel,                  /* IN What to do*/
  VOID_CALLBACKFUNC(pCbFunc)(BYTE val))
{
  WORD bat_mv;
  ZW_APPLICATION_TX_BUFFER *pTxBuf = GetRequestBuffer(pCbFunc);
  if(NULL == pTxBuf)
  {
    /*Ongoing job is active.. just stop current job*/
    return JOB_STATUS_BUSY;
  }

  pTxBuf->ZW_BatteryReportFrame.cmdClass = COMMAND_CLASS_BATTERY;
  pTxBuf->ZW_BatteryReportFrame.cmd = BATTERY_REPORT;
  pTxBuf->ZW_BatteryReportFrame.batteryLevel = bBattLevel;
  if(FALSE == Transport_SendRequest(
      dstNode,
      (BYTE *)pTxBuf,
      sizeof(ZW_BATTERY_REPORT_FRAME),
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
