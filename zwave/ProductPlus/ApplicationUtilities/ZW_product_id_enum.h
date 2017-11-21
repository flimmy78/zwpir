/****************************************************************************
 *
 * Copyright (c) 2001-2013
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: Declaration of Z-Wave Product IDs.
 *
 * All product IDs are taken from the following document:
 * Software Design Specification Document SDS11017
 * Manufacturer and Product Identifiers
 * http://highstage.sdesigns.com/item/item.aspx?ot=doc&o=SDS11017
 * These product IDs should always be in sync.
 *
 * Author:   Erik Friis Harck
 *
 * Last Changed By:  $Author: iza $
 * Revision:         $Revision: 22797 $
 * Last Changed:     $Date: 2012-05-10 15:55:06 +0200 (to, 10 maj 2012) $
 *
 ****************************************************************************/
#ifndef _PRODUCT_ID_ENUM_H_
#define _PRODUCT_ID_ENUM_H_

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

typedef enum _PRODUCT_TYPE_ID_ENUM_
{
  PRODUCT_TYPE_ID_ZWAVE_ZIP_GATEWAY = 1,
  PRODUCT_TYPE_ID_ZWAVE,
  PRODUCT_TYPE_ID_ZWAVE_PLUS
} eProductTypeID;

typedef enum _PRODUCT_PLUS_ID_ENUM_
{
  PRODUCT_ID_DoorLockKeyPad = 0x0001,
  PRODUCT_ID_SwitchOnOff = 0x0002,
  PRODUCT_ID_SensorPIR = 0x0003,
  PRODUCT_ID_InclusionController = 0x0004,
  PRODUCT_ID_MyProductPlus = 0x0005,
  PRODUCT_ID_SecureSensorPIR =  0x0006,
  PRODUCT_ID_SecureSwitchOnOff = 0x0007,
  PRODUCT_ID_ZIRC = 0x21ac
} eProductPlusID;

#endif /* _PRODUCT_ID_ENUM_H_ */
