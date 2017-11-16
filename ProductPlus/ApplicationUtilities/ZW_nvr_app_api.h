/**
 *
 * Copyright (c) 2001-2013
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 * @file ZW_nvr_app_api.h
 *
 * @brief API to NVR application area
 *
 * Author: Thomas Roll
 *
 * Last Changed By: $Author: tro $
 * Revision: $Revision: 0.00 $
 * Last Changed: $Date: 2013/09/16 15:49:32 $
 *
 */

#ifndef _ZW_nvr_app_api_H_
#define _ZW_nvr_app_api_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/


/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * Data structure for NVR application area. 
 * Max size is (NVR_APP_END_ADDRESS - NVR_APP_START_ADDRESS)
 */
typedef struct _NVR_APP_FLASH_STRUCT_
{
  BYTE hwVersion; /* product hardware version*/
  
} NVR_APP_FLASH_STRUCT;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/


/** 
 * @brief ZW_NVRGetValue
 *    Get a value from the NVR flash page application area. There is no CRC 
 *    protection of NVR application area!
 *
 * @param bOffset is the offset in NVR area. Offset 0 is NVR_APP_START_ADDRESS
 *        address ends with = (NVR_APP_END_ADDRESS - NVR_APP_START_ADDRESS).
 * @param bLength is the length of data to be read.
 * @param bRetBuffer pointer to return data.
 */
void
ZW_NVRGetAppValue(BYTE bOffset, BYTE bLength, BYTE *bRetBuffer);



#endif /* _ZW_nvr_app_api_H_ */

