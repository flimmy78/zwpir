/*************************************************************************** 
* 
* Copyright (c) 2001-2011 
* Sigma Designs, Inc. 
* All Rights Reserved 
* 
*--------------------------------------------------------------------------- 
* 
* Description: Manufacturer specific  Command Class source file
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
#include <ZW_uart_api.h>
#include "config_app.h"
#include <CommandClassManufacturerSpecific.h>
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

/*==============================   handleCommandClassManufacturerSpecific  ============
**
**  Function:  handler for Manufacturer Specific CC
**
**  Side effects: None
**
**--------------------------------------------------------------------------*/
void 
handleCommandClassManufacturerSpecific(
  BYTE  option,                 /* IN Frame header info */
  BYTE  sourceNode,               /* IN Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, the union */
  /*    should be used to access the fields */
  BYTE   cmdLength                /* IN Number of command bytes including the command */
)
{
  switch(pCmd->ZW_Common.cmd)
  {
    case MANUFACTURER_SPECIFIC_GET_V2:
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if(NULL != pTxBuf)
        {
          pTxBuf->ZW_ManufacturerSpecificReportV2Frame.cmdClass = COMMAND_CLASS_MANUFACTURER_SPECIFIC_V2;
          pTxBuf->ZW_ManufacturerSpecificReportV2Frame.cmd = MANUFACTURER_SPECIFIC_REPORT_V2;
          ApplManufacturerSpecificInfoGet(&(pTxBuf->ZW_ManufacturerSpecificReportV2Frame.manufacturerId1));
          if(FALSE == Transport_SendResponse(
              sourceNode,
              (BYTE *)pTxBuf,
              sizeof(ZW_MANUFACTURER_SPECIFIC_REPORT_V2_FRAME),
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
    case DEVICE_SPECIFIC_GET_V2:
      {
        BOOL sendFrame = FALSE;
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        /*Check pTxBuf is free*/
        if(NULL != pTxBuf)
        {
          DEVICE_ID_FORMAT DevIdDataFormat;
          BYTE DevIdDataLen;
          pTxBuf->ZW_DeviceSpecificReport1byteV2Frame.cmdClass = COMMAND_CLASS_MANUFACTURER_SPECIFIC_V2;
          pTxBuf->ZW_DeviceSpecificReport1byteV2Frame.cmd = DEVICE_SPECIFIC_REPORT_V2;

          
          sendFrame = ApplDeviceSpecificInfoGet(&pCmd->ZW_DeviceSpecificGetV2Frame.properties1,
                                    &DevIdDataFormat,
                                    &DevIdDataLen,
                                    &(pTxBuf->ZW_DeviceSpecificReport1byteV2Frame.deviceIdData1));
                                    
          pTxBuf->ZW_DeviceSpecificReport1byteV2Frame.properties1 = pCmd->ZW_DeviceSpecificGetV2Frame.properties1;
          pTxBuf->ZW_DeviceSpecificReport1byteV2Frame.properties2 = (DevIdDataFormat << 5) | (DevIdDataLen & 0x1F);
         

          if(TRUE == sendFrame)
          {
            if(FALSE == Transport_SendResponse(
                sourceNode,
                (BYTE *)pTxBuf,
                sizeof(ZW_DEVICE_SPECIFIC_REPORT_1BYTE_V2_FRAME) + 
                  (pTxBuf->ZW_DeviceSpecificReport1byteV2Frame.properties2 & 0x1F) - 1, /*Drag out length field 0x1F*/
                option,
                ZCB_ResponseJobStatus))
            {
              /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
              FreeResponseBuffer();          
            }
          }
          else{
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
