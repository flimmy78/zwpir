/**
 * @file
 * Handler for Command Class Firmware Update.
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMANDCLASSFIRMWAREUPDATE_H_
#define _COMMANDCLASSFIRMWAREUPDATE_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_tx_mutex.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * Returns the version of this CC.
 */
#define CommandClassFirmwareUpdateMdVersionGet() FIRMWARE_UPDATE_MD_VERSION_V2

typedef struct _FW_UPDATE_GET_
{
    BYTE      manufacturerId1;              /* MSB */
    BYTE      manufacturerId2;              /* LSB */
    BYTE      firmwareId1;                  /* MSB */
    BYTE      firmwareId2;                  /* LSB */
    BYTE      checksum1;                    /* MSB */
    BYTE      checksum2;                    /* LSB */
} FW_UPDATE_GET;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/
/** 
 * @brief HandleCommandClassFWUpdate
 * @param option IN Frame header info.
 * @param sourceNode IN Command sender Node ID.
 * @param pCmd IN Payload from the received frame, the union should be used to access 
 * the fields.
 * @param cmdLength IN Number of command bytes including the command.
 * @return none.
 */
void 
handleCommandClassFWUpdate(
  BYTE  option,                 /* IN Frame header info */
  BYTE  sourceNode,               /* IN Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, the union */
  /*    should be used to access the fields */
  BYTE   cmdLength                /* IN Number of command bytes including the command */
);



/** 
 * @brief handleCmdClassFirmwareUpdateMdReport
 * Application function to handle incomming frame Firmware update  MD Report
 * @param par description..
 * @return JOB_STATUS
 */
extern void
handleCmdClassFirmwareUpdateMdReport( WORD crc16Result, 
                                      WORD firmwareUpdateReportNumber,
                                      BYTE properties,
                                      BYTE* pData,
                                      BYTE fw_actualFrameSize);




/** 
 * @brief CmdClassFirmwareUpdateMdStatusReport
 * Comment function...
 * @param destNode destination node
 * @param status Values used for Firmware Update Md Status Report command 
 * FIRMWARE_UPDATE_MD_STATUS_REPORT_UNABLE_TO_RECEIVE_WITHOUT_CHECKSUM_ERROR_V2     0x00
 * FIRMWARE_UPDATE_MD_STATUS_REPORT_UNABLE_TO_RECEIVE_V2                            0x01
 * FIRMWARE_UPDATE_MD_STATUS_REPORT_SUCCESSFULLY_V2                                 0xFF
 * @param pCbFunc function pointer retunrning status on job.
 * @return JOB_STATUS..
 */
JOB_STATUS
CmdClassFirmwareUpdateMdStatusReport(BYTE destNode, BYTE status, BYTE txOption, VOID_CALLBACKFUNC(pCbFunc)(BYTE val));

/** 
 * @brief CmdClassFirmwareUpdateMdGet
 * Send command Firmware update  MD Get
 * @param destNode destination node
 * @param firmwareUpdateReportNumber current frame number.
 * @return JOB_STATUS
 */
JOB_STATUS
CmdClassFirmwareUpdateMdGet(BYTE destNode, WORD firmwareUpdateReportNumber, BYTE txOption);


/** 
 * @brief RemoteReqQuthentication
 * Remote request for firmware update
 * @param par description..
 * @return TRUE: we are ready to firmware update. FALSE: reject it.
 */
extern BOOL
RemoteReqAuthentication();


/** 
 * @brief handleCmdClassFirmwareUpdateMdReqGet
 * Comment function...
 * @param par description..
 * @return description..
 */
extern void
handleCmdClassFirmwareUpdateMdReqGet( 
  BYTE node, 
  FW_UPDATE_GET* pData, 
  BYTE* pStatus);
   

/** 
 * @brief ZCB_CmdClassFwUpdateMdReqReport
 * Callback function receive status on Send data FIRMWARE_UPDATE_MD_REQUEST_REPORT_V2
 * @param val status: TRANSMIT_COMPLETE_OK, TRANSMIT_COMPLETE_NO_ACK, TRANSMIT_COMPLETE_FAIL...
 * @return description..
 */
extern void
ZCB_CmdClassFwUpdateMdReqReport(BYTE txStatus);
                                      
#endif /* _COMMANDCLASSFIRMWAREUPDATE_H_*/

