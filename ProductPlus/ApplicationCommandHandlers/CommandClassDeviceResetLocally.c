/***************************************************************************
*
* Copyright (c) 2001-2011
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Device reset locally Command Class source file
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
#include <ZW_transport_api.h>
#include <ZW_tx_mutex.h>

#include "config_app.h"
#include <CommandClassDeviceResetLocally.h>
#include <ZW_slave_routing_api.h>
#include <ZW_slave_api.h>

#include <misc.h>
#include <association_plus.h>

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



/*==============================   handleCommandClassDeviceResetLocally  ============
**
**  Function:  handler for Device Reset Locally CC
**
**  Side effects: None
**
**--------------------------------------------------------------------------*/
void
handleCommandClassDeviceResetLocally( VOID_CALLBACKFUNC(completedFunc)(BYTE))
{
  BYTE dest;
  ZW_APPLICATION_TX_BUFFER *pTxBuf = GetRequestBuffer(completedFunc);

  if(NULL == pTxBuf)
  {
    /*Ongoing job is active or GetMyNodeID is 0 .. just stop current cmdClass job and reset.*/
    completedFunc(TRANSMIT_COMPLETE_FAIL);
  }
  else if(!GetMyNodeID())
  {
    /*Clear transmit-buffer mutex and inform application it was failing!*/
    ZCB_RequestJobStatus(TRANSMIT_COMPLETE_FAIL);
  }
  else
  {
    dest = AssociationGetLifeLineNodeID();
    pTxBuf->ZW_DeviceResetLocallyNotificationFrame.cmdClass = COMMAND_CLASS_DEVICE_RESET_LOCALLY;
    pTxBuf->ZW_DeviceResetLocallyNotificationFrame.cmd = DEVICE_RESET_LOCALLY_NOTIFICATION;

    if(FALSE == Transport_SendRequest(
      dest,
      (BYTE *)pTxBuf,
      sizeof(ZW_DEVICE_RESET_LOCALLY_NOTIFICATION_FRAME),
      ZWAVE_PLUS_TX_OPTIONS,
      ZCB_RequestJobStatus,
      FALSE))
    {
      /*Free transmit-buffer mutex and tell application it can reset.*/
      ZCB_RequestJobStatus(TRANSMIT_COMPLETE_FAIL);
    }
  }
}
