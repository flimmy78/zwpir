/****************************************************************************
 *
 * Copyright (c) 2001-2011
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: NVM layout for all node types
 *
 * Author:  Erik Friis Harck
 *
 * Last Changed By:  $Author: efh $
 * Revision:         $Revision: 11509 $
 * Last Changed:     $Date: 2012-03-05 10:45:45 +0200 (Wed, 5 Mar 2012) $
 *
 ****************************************************************************/

/* EEPROM binary battery sensor node layout */
/* Make sure compiler won't shuffle around with these variables,            */
/* as there are external dependencies.                                      */
/* All these variables needs to initialized, because the compiler groups    */
/* the variables into different classes for uninitialized/initialized       */
/* when ordering them. To keep them in order... Keep them all initialized.  */
#pragma ORDER

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <ZW_nvr_api.h>
#include "config_app.h"
#include <manufacturer_specific_device_id.h>
#include <eeprom.h>
/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

#define MAGIC_VALUE  0x42
/*WARNING the UUID_NVR_OFFSET should not by hardcoded as below since it it based on a structure that can
  changed it formate this is a temporary soultuion as the structure is not globally available yet*/
#define UUID_NVR_OFFSET  13
BYTE manSpecificDeviceID[MAN_DEVICE_ID_SIZE];
static BYTE tmp_i;
/***********************/
/*      NVM layout     */
/***********************/



static BOOL
CheckDeviceID(BYTE * deviceID)
{
  /*The device ID should not be a repeated value.*/
  /*test if the device Id is all 0xFF or 0x00*/
  BYTE tmp = deviceID[0];
  for( tmp_i = 1; tmp_i < sizeof(MAN_DEVICE_ID_SIZE); tmp_i++)
  {
    tmp &= deviceID[tmp_i];
  }
  if (tmp == 0xFF)
    return FALSE;

  tmp = deviceID[0];
  for( tmp_i = 1; tmp_i < sizeof(MAN_DEVICE_ID_SIZE); tmp_i++)
  {
    tmp |= deviceID[tmp_i];
  }
  if (!tmp )
    return FALSE;

  return TRUE;

}
/*======================= ManufcaturerSpecificDeviceIDInit =======================
** Function description
** Read the manufacturer specific unique serial number from NVR
**  if the number from NVR all 0xFF then create a random one.
**
** Return none
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
ManufacturerSpecificDeviceIDInit()
{
 /*Read UUID from NVR*/
  ZW_NVRGetValue(offsetof(NVR_FLASH_STRUCT, abUUID), MAN_DEVICE_ID_SIZE, manSpecificDeviceID);
  /* if the UUID is leagal use it as the manufacturer specific device ID
     else create a random Id and save it in the eeprom */
  if (!CheckDeviceID(manSpecificDeviceID))
  {

    if (MemoryGetByte((WORD)&nvmApplDescriptor.EEOFFSET_MAN_SPECIFIC_MAGIC_far) == MAGIC_VALUE)
    {/*we already created a random Id then use it*/
      MemoryGetBuffer((WORD)&nvmApplDescriptor.EEOFFSET_MAN_SPECIFIC_DEVICE_ID_far,
                       manSpecificDeviceID, MAN_DEVICE_ID_SIZE);
    }
    else
    {
    /*UUID is illegal create a random ID*/
      MemoryPutByte((WORD)&nvmApplDescriptor.EEOFFSET_MAN_SPECIFIC_MAGIC_far, MAGIC_VALUE);
      do
      {
        for (tmp_i = 0; tmp_i < MAN_DEVICE_ID_SIZE; tmp_i+=2)
          ZW_GetRandomWord(&manSpecificDeviceID[tmp_i]);
        /*The device ID should not be a repeated value.*/
        /*test if the device Id is all 0xFF or 0x00*/
      }
      while(!CheckDeviceID(manSpecificDeviceID));
      ZW_MemoryPutBuffer((WORD)&nvmApplDescriptor.EEOFFSET_MAN_SPECIFIC_DEVICE_ID_far,
                         manSpecificDeviceID, MAN_DEVICE_ID_SIZE);

    }
  }
}

/*======================= ManufcaturerSpecificDeviceIDGet =======================
** Function description
** Copy the manufacturer device id
*
** Return none
**
** Side effects:
**
**-------------------------------------------------------------------------*/
BYTE                            /*RET manufacturer specific device ID length*/
ManufacturerSpecificDeviceIDGet(BYTE *deviceID)
{
  for (tmp_i = 0; tmp_i < MAN_DEVICE_ID_SIZE; tmp_i++)
  {
    deviceID[tmp_i] = manSpecificDeviceID[tmp_i];
  }
  return MAN_DEVICE_ID_SIZE;
}

/*============================ GetApplManufacturerSpecificInfo =============
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
void
ApplManufacturerSpecificInfoGet(
  T_MSINFO *pMsInfo)
{
  pMsInfo->manufacturerId1 = (APP_MANUFACTURER_ID >> 8);
  pMsInfo->manufacturerId2 = (APP_MANUFACTURER_ID & 0xFF);
  pMsInfo->productTypeId1 = (APP_PRODUCT_TYPE_ID >> 8);
  pMsInfo->productTypeId2 = (APP_PRODUCT_TYPE_ID & 0xFF);
  pMsInfo->productId1 = (APP_PRODUCT_ID>>8);
  pMsInfo->productId2 = (APP_PRODUCT_ID & 0xFF);

}


/*============================ ApplDeviceSpecificInfoGet ===================
** Function description
** This function...
**
** Side effects:
**
**-------------------------------------------------------------------------*/
BOOL
ApplDeviceSpecificInfoGet(DEVICE_ID_TYPE *deviceIdType,
  DEVICE_ID_FORMAT* pDevIdDataFormat,
  BYTE* pDevIdDataLen,
  BYTE* pDevIdData)
{
  if(*deviceIdType == APP_DEVICE_ID_TYPE || *deviceIdType == DEVICE_ID_TYPE_OEM)
  {
    *deviceIdType = APP_DEVICE_ID_TYPE;
    *pDevIdDataFormat = APP_DEVICE_ID_FORMAT;
    *pDevIdDataLen = ManufacturerSpecificDeviceIDGet(pDevIdData);
    return TRUE;
  }
  else
  {
    *deviceIdType = 0;
    *pDevIdDataFormat = 0;
    *pDevIdDataLen = 0;
  }
  return FALSE;
}
