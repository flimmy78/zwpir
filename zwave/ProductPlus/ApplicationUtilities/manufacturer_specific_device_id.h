/**
 *
 * Copyright (c) 2001-2014
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 * @file manufacturer_specific_device_id.h
 *
 * @brief Configuration of manufacturer specific device Id.
 *        All far variables (NVM offsets) should be defined in the application's eeprom.h module
 *        in the struct t_nvmApplDescriptor
 *
 * Author: Thomas Roll
 *
 * Last Changed By: $Author: tro $
 * Revision: $Revision: 0.00 $
 * Last Changed: $Date: 2014/05/21 13:32:55 $
 *
 */

#ifndef _MANUFACTURER_SPECIFIC_DEVICE_ID_H_
#define _MANUFACTURER_SPECIFIC_DEVICE_ID_H_


/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_sysdefs.h>
#include <ZW_classcmd.h>
#include <CommandClassManufacturerSpecific.h>

#define MAN_DEVICE_ID_SIZE  8

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/



/**
 * @brief ManufacturerSpecificDeviceIDInit
 * Read the manufacturer specific unique serial number from NVR
 * if the number from NVR all 0xFF then create a random one.
 */
void
ManufacturerSpecificDeviceIDInit();


#endif /*#ifndef  _MANUFACTURER_SPECIFIC_DEVICE_ID_H_*/
