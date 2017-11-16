/**
 * @file
 * Handler for Command Class Version.
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMAND_CLASS_VERSION_H_
#define _COMMAND_CLASS_VERSION_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_sysdefs.h>
#include <ZW_pindefs.h>
#include <ZW_evaldefs.h>
#include <ZW_classcmd.h>

/**
 * Returns the version of this CC.
 */
#define CommandClassVersionVersionGet() VERSION_VERSION_V2

/*==============================   handleCommandClassVersion  ============
**
**  Function:  handler for App specific part of Version CC
**
**  Side effects: None
**
**--------------------------------------------------------------------------*/
extern BYTE 
handleCommandClassVersionAppl(BYTE);

/*==============================   handleCommandClassVersion  ============
**
**  Function:  handler for Version CC
**
**  Side effects: None
**
**--------------------------------------------------------------------------*/
extern void 
handleCommandClassVersion(
  BYTE  option,                      /* IN Frame header info */
  BYTE  sourceNode,                  /* IN Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER *pCmd,    /* IN Payload from the received frame, the union */
  /*    should be used to access the fields */
  BYTE   cmdLength                   /* IN Number of command bytes including the command */
);

/** 
 * @brief handleNbrFirmwareVersions
 * Return number (N) of firmware versions.
 * @return N
 */
extern BYTE
handleNbrFirmwareVersions(void);

/** 
 * @brief handleGetFirmwareVersion
 * Comment function...
 * @param N read version number n (0,1..N-1)
 * @param pVersion returns the Firmware n Version. Firmware n is dedicated to the 
 * Z-Wave chip firmware. The manufacturer MUST assign a version number.
 * @param pVariantgroup returns pointer to application version group number n.
 */
extern void
handleGetFirmwareVersion( BYTE n, VG_VERSION_REPORT_V2_VG* pVariantgroup);

/** 
 * @brief handleGetFirmwareHwVersion
 * The Hardware Version field MUST report a value which is unique to this particular 
 * version of the product. It MUST be possible to uniquely determine the hardware 
 * characteristics from the Hardware Version field in combination with the Manufacturer 
 * ID, Product Type ID and Product ID fields of Manufacturer Specific Info Report 
 * of the Manufacturer Specific Command Class.
 * This information allows a user to pick a firmware image version that is guaranteed 
 * to work with this particular version of the product.
 * Note that the Hardware Version field is intended for the hardware version of the 
 * entire product, not just the version of the Z-Wave radio chip
 * @return Hardware version
 */
extern BYTE
handleGetFirmwareHwVersion(void);

#endif /*_COMMAND_CLASS_VERSION_H_*/

