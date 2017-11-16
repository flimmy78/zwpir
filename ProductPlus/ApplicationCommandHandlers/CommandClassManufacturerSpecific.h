/**
 * @file
 * Handler for Command Class Manufacturer Specific.
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMANDCLASSMANUFACTURERSPECIFIC_H_
#define _COMMANDCLASSMANUFACTURERSPECIFIC_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_sysdefs.h>
#include <ZW_pindefs.h>
#include <ZW_evaldefs.h>
#include <ZW_classcmd.h>


/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * Returns the version of this CC.
 */
#define CommandClassManufacturerVersionGet() MANUFACTURER_SPECIFIC_VERSION_V2

typedef struct _T_MSINFO_{
  BYTE  manufacturerId1;
  BYTE  manufacturerId2;
  BYTE  productTypeId1;
  BYTE  productTypeId2;
  BYTE  productId1;
  BYTE  productId2;
} T_MSINFO;


typedef enum _DEVICE_ID_TYPE_ { DEVICE_ID_TYPE_OEM = 0, DEVICE_ID_TYPE_SERIAL_NBR, DEVICE_ID_TYPE_PSEUDO_RANDOM} DEVICE_ID_TYPE;
typedef enum _DEVICE_ID_FORMAT_ { DEVICE_ID_FORMAT_UTF_8 = 0, DEVICE_ID_FORMAT_BIN} DEVICE_ID_FORMAT;
typedef struct _DEV_ID_DATA
{
  BYTE DevIdDataFormat: 3; /*Type DEVICE_ID_FORMAT*/
  BYTE DevIdDataLen: 5;
  BYTE* pDevIdData;
} DEV_ID_DATA;
/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

 
/** 
 * @brief handleCommandClassManufacturerSpecific
 * @param option IN Frame header info.
 * @param sourceNode IN Command sender Node ID.
 * @param pCmd IN Payload from the received frame, the union should be used to access 
 * the fields.
 * @param cmdLength IN Number of command bytes including the command.
 * @return none.
 */
extern void
handleCommandClassManufacturerSpecific(
  BYTE  option,                    
  BYTE  sourceNode,                
  ZW_APPLICATION_TX_BUFFER *pCmd,  
  BYTE   cmdLength                
);

/** 
 * @brief ApplManufacturerSpecificInfoGet
 * Read the manufacturer specific info from the application the manufacturer 
 * specific info are the manufacturerId1/2, productTypeId1/2 and productId1/2.
 *
 * @param msInfo: OUT pointer of type t_MSInfo that should hold the manufacturer 
 * specific information
 * @return None.
 */
extern void 
ApplManufacturerSpecificInfoGet(T_MSINFO *t_msInfo); 


/** 
 * @brief ApplDeviceSpecificInfoGet
 * Read the Device specifict ID Data fields.
 * @param deviceIdType values for the Device ID Type of enum type DEVICE_ID_TYPE
 * @param pDevIdData OUT pointer to the Device ID Data fields.
 * @param pDevIdDataLen OUT pointer returning len of the Device ID Data fields.
 * @return none.
 */
extern BOOL
ApplDeviceSpecificInfoGet(DEVICE_ID_TYPE *deviceIdType, 
  DEVICE_ID_FORMAT* pDevIdDataFormat,
  BYTE* pDevIdDataLen,
  BYTE* pDevIdData);

#endif /* _COMMANDCLASSMANUFACTURERSPECIFIC_H_ */

