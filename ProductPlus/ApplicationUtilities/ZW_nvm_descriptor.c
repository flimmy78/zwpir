/****************************************************************************
 *
 * Copyright (c) 2001-2013
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: Definition and initialization of Z-Wave NVM descriptor.
 *
 * Author:   Erik Friis Harck
 *
 * Last Changed By:  $Author: iza $
 * Revision:         $Revision: 22797 $
 * Last Changed:     $Date: 2012-05-10 15:55:06 +0200 (to, 10 maj 2012) $
 *
 ****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "config_app.h"
#include <ZW_basis_api.h>
#include <ZW_nvm_descriptor.h>


/****************************************************************************/
/*                              EXTERNALS                                   */
/****************************************************************************/
extern unsigned char _APP_VERSION_;
extern unsigned char _ZW_VERSION_;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/* NVM descriptor for firmware */
t_nvmDescriptor far nvmDescriptor =
{
  /* TODO: Fill in your unique and assigned manufacturer ID here                */
  APP_MANUFACTURER_ID,                          /* WORD manufacturerID;         */
  /* TODO: Fill in your own unique firmware ID here                             */
  APP_FIRMWARE_ID,                              /* WORD firmwareID;             */
  /* TODO: Fill in your own unique Product Type ID here                         */
  APP_PRODUCT_TYPE_ID,                          /* WORD productTypeID;          */
  /* TODO: Fill in your own unique Product ID here                              */
  APP_PRODUCT_ID,                               /* WORD productID;              */
  /* Unique Application Version (from config_app.h)                             */
  (WORD)&_APP_VERSION_,                         /* WORD applicationVersion;     */
  /* Unique Z-Wave protocol Version (from config_lib.h)                         */
  (WORD)&_ZW_VERSION_,                          /* WORD zwaveProtocolVersion;   */
};

