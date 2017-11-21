/**
 * @file
 * Handler for Command Class Binary Switch.
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMAND_CLASS_BINARY_SWITCH_H_
#define _COMMAND_CLASS_BINARY_SWITCH_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_sysdefs.h>
#include <ZW_pindefs.h>
#include <ZW_evaldefs.h>
#include <ZW_classcmd.h>
#include <ZW_tx_mutex.h>

/**
 * Returns the version of this CC.
 */
#define CommandClassBinarySwitchVersionGet() SWITCH_BINARY_VERSION

/**
 * The value can be either 0x00 (off/disable) or 0xFF (on/enable). The values from 
 * 1 to 99 (0x01 to 0x63) SHALL mapped to 0xFF upon receipt of the Command in the 
 * device. All other values are reserved and SHALL be ignored by the receiving device.
 */
typedef enum { 
  CMD_CLASS_BIN_OFF = 0x00, /**< off/disable */ //!< CMD_CLASS_BIN_OFF
  CMD_CLASS_BIN_ON  = 0xFF  /**< on/enable */   //!< CMD_CLASS_BIN_ON
} CMD_CLASS_BIN_SW_VAL;

/*==============================   sendApplReport  ===================================
**
** 
**  Side effects: None
**
**-----------------------------------------------------------------------------------*/
/*============================ handleAppltBinarySwitchGet ===============================
** Function description
** Application Get-function called on incoming frame CommandClassBinarySwitch-Get.
**
** Side effects: 
**
**-------------------------------------------------------------------------*/
extern BYTE
handleAppltBinarySwitchGet(void);

/*======================   handleApplBinarySwitchSet  ===============================
**
** 
**  Side effects: None
**
**-----------------------------------------------------------------------------------*/
extern void 
handleApplBinarySwitchSet(
  CMD_CLASS_BIN_SW_VAL val
);

/*==============================   handleCommandClassBinarySwitch  ============
**
**  Function:  handler for Binary Switch Info CC
**
**  Side effects: None
**
**--------------------------------------------------------------------------*/
extern void 
handleCommandClassBinarySwitch(
  BYTE  option,                 /* IN Frame header info */
  BYTE  sourceNode,               /* IN Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, the union */
  /*    should be used to access the fields */
  BYTE   cmdLength                /* IN Number of command bytes including the command */
);

/*============================ ZCB_CmdClassBinarySwitchSupportSet ===========
** Function description
** Check value is correct for current class and call application Set function.
**
** Side effects: 
**
**-------------------------------------------------------------------------*/
void 
ZCB_CmdCBinarySwitchSupportSet( BYTE val);

/** 
 * @brief CmdClassBinarySwitchReportSendUnsolicited
 * Send a unsolicited binary Switch report.
 * @param option Transmit options
 * @param dstNode destination node
 * @param bValue report value of enum type CMD_CLASS_BIN_SW_VAL
 * @param (pCbFunc)(BYTE val) callback funtion returning status destination node receive job.
 * @return status on protocol exuted the job.
 */
JOB_STATUS
CmdClassBinarySwitchReportSendUnsolicited(
  BYTE option,
  BYTE dstNode,
  CMD_CLASS_BIN_SW_VAL bValue,
  VOID_CALLBACKFUNC(pCbFunc)(BYTE val));



#endif
