/**
 * @file
 * Handler for Command Class Basic.
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMAND_CLASS_BASIC_H_
#define _COMMAND_CLASS_BASIC_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_classcmd.h>
#include <ZW_tx_mutex.h>

/**
 * Returns the version of this CC.
 */
#define CommandClassBasicVersionGet() BASIC_VERSION

/*=========================   handleBasicSetCommand  ===============================
**
** 
**  Side effects: None
**
**-----------------------------------------------------------------------------------*/
extern void 
handleBasicSetCommand(
  BYTE val
);

/*==============================   getAppBasicReport  ===============================
**
** 
**  Side effects: None
**
**-----------------------------------------------------------------------------------*/
extern BYTE 
getAppBasicReport(void);

/*==============================   handleCommandClassBasic  ============
**
**  Function:  handler for Basic CC
**
**  Side effects: None
**
**--------------------------------------------------------------------------*/
extern void 
handleCommandClassBasic(
  BYTE  option,                 /* IN Frame header info */
  BYTE  sourceNode,               /* IN Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, the union */
  /*    should be used to access the fields */
  BYTE   cmdLength                /* IN Number of command bytes including the command */
);

/*========================   CmdClassBasicSetSend  ===============
**
**  Function:  Send basic set frame
**
**  Side effects: JOB_STATUS
**
**--------------------------------------------------------------------------*/

JOB_STATUS
CmdClassBasicSetSend(
  BYTE option,
  BYTE dstNode,
  BYTE bValue,
  VOID_CALLBACKFUNC(pCbFunc)(BYTE val)
);

/** 
 * @brief CmdClassBasicReportSend
 * Send unsolicited Basic report
 * @param option Frame header info
 * @param dstNode destination node ID
 * @param bValue Basic Report value
 * @param pCbFunc callback function pointer returning status on job. Can be initialiazed to NULL!
 * @return description..
 */
JOB_STATUS
CmdClassBasicReportSend(
  BYTE option,
  BYTE dstNode,
  BYTE bValue,
  VOID_CALLBACKFUNC(pCbFunc)(BYTE val));

#endif
