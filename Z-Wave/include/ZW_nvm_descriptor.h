/****************************************************************************
 *
 * Copyright (c) 2001-2012
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: Declaration of Z-Wave NVM descriptor.
 *
 * Author:   Erik Friis Harck
 *
 * Last Changed By:  $Author: iza $
 * Revision:         $Revision: 22797 $
 * Last Changed:     $Date: 2012-05-10 15:55:06 +0200 (to, 10 maj 2012) $
 *
 ****************************************************************************/
#ifndef _NVM_DESCRIPTOR_H_
#define _NVM_DESCRIPTOR_H_

#include <ZW_typedefs.h>

/****************************************************************************/
/*                              EXTERNALS                                   */
/****************************************************************************/

/* Make APP_VERSION and APP_REVISION public, so that Z-Wave protocol code can access it. */
/* Use it from C-code like this:                                                         */
extern unsigned char _APP_VERSION_;     /* referenced with = (WORD)&_APP_VERSION_);      */
extern unsigned char _APP_REVISION_;    /* referenced with = (WORD)&_APP_REVISION_);     */

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/* NVM descriptor for OTA firmware update. Located at the end of NVM. */
typedef struct s_nvmDescriptor_
{
  WORD manufacturerID;
  WORD firmwareID;
  WORD productTypeID;
  WORD productID;
  WORD applicationVersion;
  WORD zwaveProtocolVersion;
} t_nvmDescriptor;

extern t_nvmDescriptor far nvmDescriptor;

#endif /* _NVM_DESCRIPTOR_H_ */
