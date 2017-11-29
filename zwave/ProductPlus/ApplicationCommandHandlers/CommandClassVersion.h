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
#include <CommandClass.h>
#include <ZW_TransportEndpoint.h>

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
/**
 * @brief handleCommandClassVersionAppl
 * Read command class version
 * @param cmdClass command class
 * @return version
 */
extern BYTE handleCommandClassVersionAppl(BYTE cmdClass);

/*==============================   handleCommandClassVersion  ============
**
**  Function:  handler for Version CC
**
**  Side effects: None
**
**--------------------------------------------------------------------------*/
/**
 * @brief handleCommandClassVersion
 * handler for call command class modules version functions
 * @param[in] rxOpt pointer to rx options
 * @param[in] pCmd pointer to command
 * @param[in] cmdLength length of command
 * @return receive frame status.
 */
received_frame_status_t handleCommandClassVersion(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength);

/**
 * @brief handleNbrFirmwareVersions
 * Return number (N) of firmware versions.
 * @return N
 */
extern BYTE handleNbrFirmwareVersions(void);

/**
 * @brief handleGetFirmwareVersion
 * Get application firmware verions
 * @param[in] n read version number n (0,1..N-1)
 * @param pVariantgroup returns pointer to application version group number n.
 */
extern void handleGetFirmwareVersion( BYTE n, VG_VERSION_REPORT_V2_VG* pVariantgroup);

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
extern BYTE handleGetFirmwareHwVersion(void);

#endif /*_COMMAND_CLASS_VERSION_H_*/

