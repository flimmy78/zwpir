/***************************************************************************
*
* Copyright (c) 2001-2013
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: The Door Lock Command Class used to secure/unsecure a lock
*              type as well as setting the configuration of an advanced
*              Z-Wave™ door lock device. Version 2 enable Door Lock
*              Operation Report Command to report a Door Lock Mode that the
*              door/lock state is unknown, i.e. bolt is not fully
*              retracted/engaged.
*
*        Door Lock Command Class, version 1-2.
*        Z-Wave command Class Specification SDS11060.doc
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2013/05/02 09:38:04 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <ZW_TransportLayer.h>
#include "config_app.h"
#include <CommandClassDoorLock.h>
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



/*============================ handleCommandClassDoorLock ===================
** Function description
** Handle incoming command class doorlock frames version 1 & 2.
**
** Side effects: none
**
**-------------------------------------------------------------------------*/
void
handleCommandClassDoorLock(
  BYTE  option,                 /* IN Frame header info */
  BYTE  sourceNode,               /* IN Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, the union */
  /*    should be used to access the fields */
  BYTE   cmdLength                /* IN Number of command bytes including the command */
)
{

  switch (pCmd->ZW_Common.cmd)
  {

    case DOOR_LOCK_OPERATION_SET_V2:
      handleCommandClassDoorLockOperationSet(pCmd->ZW_DoorLockOperationSetV2Frame.doorLockMode);
      break;

    case DOOR_LOCK_OPERATION_GET_V2:
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if(NULL != pTxBuf)
        {
          pTxBuf->ZW_DoorLockOperationReportV2Frame.cmdClass = COMMAND_CLASS_DOOR_LOCK_V2;
          pTxBuf->ZW_DoorLockOperationReportV2Frame.cmd = DOOR_LOCK_OPERATION_REPORT_V2;
          handleCommandClassDoorLockOperationReport( &(pTxBuf->ZW_DoorLockOperationReportV2Frame.doorLockMode));
          if(FALSE == Transport_SendResponse(
              sourceNode,
              (BYTE *)pTxBuf,
              sizeof(ZW_DOOR_LOCK_OPERATION_REPORT_V2_FRAME),
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

    case DOOR_LOCK_CONFIGURATION_SET_V2:
      handleCommandClassDoorLockConfigurationSet( &(pCmd->ZW_DoorLockConfigurationSetV2Frame.operationType));
      break;

    case DOOR_LOCK_CONFIGURATION_GET_V2:
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if(NULL != pTxBuf)
        {
          pTxBuf->ZW_DoorLockConfigurationReportV2Frame.cmdClass = COMMAND_CLASS_DOOR_LOCK_V2;
          pTxBuf->ZW_DoorLockConfigurationReportV2Frame.cmd = DOOR_LOCK_CONFIGURATION_REPORT_V2;
          handleCommandClassDoorLockConfigurationReport(&(pTxBuf->ZW_DoorLockConfigurationReportV2Frame.operationType));
          if(FALSE == Transport_SendResponse(
              sourceNode,
              (BYTE *)pTxBuf,
              sizeof(ZW_DOOR_LOCK_CONFIGURATION_REPORT_V2_FRAME),
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



/*============================ CmdClassDoorLockOperationSupportReport =======
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
JOB_STATUS
CmdClassDoorLockOperationSupportReport(
  BYTE destNode,
  CMD_CLASS_DOOR_LOCK_OPERATION_REPORT* pData ,
  VOID_CALLBACKFUNC(pCbFunc)(BYTE val))
{
  ZW_APPLICATION_TX_BUFFER *pTxBuf = GetRequestBuffer(pCbFunc);

  if(NULL == pTxBuf)
  {
    /*Ongoing job is active.. just stop current job*/
    return JOB_STATUS_BUSY;
  }

  pTxBuf->ZW_DoorLockOperationReportV2Frame.cmdClass      = COMMAND_CLASS_DOOR_LOCK_V2;
  pTxBuf->ZW_DoorLockOperationReportV2Frame.cmd           = DOOR_LOCK_OPERATION_REPORT_V2;
  pTxBuf->ZW_DoorLockOperationReportV2Frame.doorLockMode  = pData->mode;
  pTxBuf->ZW_DoorLockOperationReportV2Frame.properties1   = (pData->insideDoorHandleMode & 0xf) |
    (pData->outsideDoorHandleMode << 4);
  pTxBuf->ZW_DoorLockOperationReportV2Frame.doorCondition = pData->condition;
  pTxBuf->ZW_DoorLockOperationReportV2Frame.lockTimeoutMinutes  = pData->lockTimeoutMin;
  pTxBuf->ZW_DoorLockOperationReportV2Frame.lockTimeoutSeconds  = pData->lockTimeoutSec;
  if(! Transport_SendRequest(
        destNode,
        (BYTE*)pTxBuf,
        sizeof(ZW_DOOR_LOCK_OPERATION_REPORT_V2_FRAME),
        ZWAVE_PLUS_TX_OPTIONS,
        ZCB_RequestJobStatus, FALSE))
  {
    FreeRequestBuffer();
    return JOB_STATUS_BUSY;
  }

  return JOB_STATUS_SUCCESS;
}
