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
#include <CommandClassSwitchAll.h>
#include <eeprom.h>
#include <misc.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
static BYTE switchAllMode;
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
received_frame_status_t
handleCommandClassSwitchAll(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt, /* IN receive options of type RECEIVE_OPTIONS_TYPE_EX  */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN  Payload from the received frame */
  BYTE cmdLength)               /* IN Number of command bytes including the command */
{
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
  RxToTxOptions(rxOpt, &pTxOptionsEx);
  UNUSED(cmdLength);

  switch (pCmd->ZW_Common.cmd)
  {

    case SWITCH_ALL_GET:
      if(FALSE == Check_not_legal_response_job(rxOpt))
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if( NON_NULL( pTxBuf ) )
        {
          pTxBuf->ZW_SwitchAllReportFrame.cmdClass = COMMAND_CLASS_SWITCH_ALL;
          pTxBuf->ZW_SwitchAllReportFrame.cmd = SWITCH_ALL_REPORT;
          pTxBuf->ZW_SwitchAllReportFrame.mode = MemoryGetByte(  (WORD)&EEOFFSET_SWITCH_ALL_MODE_far[pTxOptionsEx->sourceEndpoint] );
          if(ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
              (BYTE *)pTxBuf,
              sizeof(ZW_SWITCH_ALL_REPORT_FRAME),
              pTxOptionsEx,
              ZCB_ResponseJobStatus))
          {
            /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
            FreeResponseBuffer();
          }
          return RECEIVED_FRAME_STATUS_SUCCESS;
        }
        return RECEIVED_FRAME_STATUS_FAIL;
      }
      break;

    case SWITCH_ALL_SET:
      MemoryPutByte(  (WORD)&EEOFFSET_SWITCH_ALL_MODE_far[pTxOptionsEx->sourceEndpoint] ,pCmd->ZW_SwitchAllSetFrame.mode);
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;

    case SWITCH_ALL_OFF:
      switchAllMode =  MemoryGetByte( (WORD)&EEOFFSET_SWITCH_ALL_MODE_far[pTxOptionsEx->sourceEndpoint]);
      if (switchAllMode & SWITCH_ALL_REPORT_EXCLUDED_FROM_THE_ALL_ON_FUNCTIONALITY_BUT_NOT_ALL_OFF)
      {
        handleSwitchAll(CMD_CLASS_SWITCHALL_OFF, pTxOptionsEx->sourceEndpoint);
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;

    case SWITCH_ALL_ON:
      switchAllMode =  MemoryGetByte( (WORD)&EEOFFSET_SWITCH_ALL_MODE_far[pTxOptionsEx->sourceEndpoint]);
      if (switchAllMode & SWITCH_ALL_REPORT_EXCLUDED_FROM_THE_ALL_OFF_FUNCTIONALITY_BUT_NOT_ALL_ON)
      {
        handleSwitchAll(CMD_CLASS_SWITCHALL_ON, pTxOptionsEx->sourceEndpoint);
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
      break;

    default:
      break;
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}
