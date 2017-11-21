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
#include <eeprom.h>
#include <ZW_nvm_descriptor.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
extern unsigned char _ZW_VERSION_;
/***********************/
/*      NVM layout     */
/***********************/
/* EEPROM binary battery sensor node layout */

  t_nvmApplDescriptor far nvmApplDescriptor;

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

