/**
 * @file
 * Handler for Command Class Battery.
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMAND_CLASS_BATTERY_H_
#define _COMMAND_CLASS_BATTERY_H_

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
#define CommandClassBatteryVersionGet() BATTERY_VERSION

/*==============================   handleCommandClassBattery  ============
**
**  Function:  handler for Battery CC
**
**  Side effects: None
**
**--------------------------------------------------------------------------*/
extern void 
handleCommandClassBattery(
  BYTE  option,                 /* IN Frame header info */
  BYTE  sourceNode,               /* IN Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, the union */
  /*    should be used to access the fields */
  BYTE   cmdLength                /* IN Number of command bytes including the command */
);

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
  VOID_CALLBACKFUNC(pCbFunc)(BYTE val));


/*==============================   BatterySensorRead   ============================
**
**  Function:  Required function for the Battery Command clas
**
**  This funciton read the Battery volatge from the battery Voltage sensor HW
**  Side effects: None
**
**--------------------------------------------------------------------------*/
BOOL 
BatterySensorRead(BYTE *battLvl);

#endif
