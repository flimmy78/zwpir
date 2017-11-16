/****************************************************************************
 *
 * Copyright (c) 2001-2011
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: EEPROM address definitions
 *
 *        All far variables (NVM offsets) should be defined in the application's eeprom.h module
 *        in the struct t_nvmApplDescriptor
 *
 * Author:   Peter Shorty
 *
 * Last Changed By:  $Author: sse $
 * Revision:         $Revision: 8501 $
 * Last Changed:     $Date: 2007-01-29 15:03:16 +0200 (Пн, 29 Січ 2007) $
 *
 ****************************************************************************/
#ifndef _EEPROM_H_
#define _EEPROM_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include "config_app.h"
#include <association_plus.h>
#include <manufacturer_specific_device_id.h>
#include <battery_plus.h>
#include <CommandClassWakeUp.h>
#include <ZW_TransportLayer.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

// ASSOCIATION_SIZE_NVM_MAX is the product of MAX_ASSOCIATION_GROUPS and MAX_ASSOCIATION_IN_GROUP
// SDK 6.51.09 is MAX_ASSOCIATION_GROUPS=4 -> 4*5 = 20
#define ASSOCIATION_SIZE_NVM_MAX    (20)

#if ASSOCIATION_SIZE > ASSOCIATION_SIZE_NVM_MAX
#error "ASSOCIATION_SIZE cannot be larger than ASSOCIATION_SIZE_NVM_MAX"
#endif

/* NVM binary battery sensor node layout */
/* NVM descriptor for application nvm*/
typedef struct s_nvmApplDescriptor_
{
  BYTE  EEOFFSET_ASSOCIATION_START_far[ASSOCIATION_SIZE_NVM_MAX];
  BYTE  EEOFFSET_ASSOCIATION_MAGIC_far;
  BYTE  EEOFFSET_MAN_SPECIFIC_DEVICE_ID_far[MAN_DEVICE_ID_SIZE];
  BYTE  EEOFFSET_MAN_SPECIFIC_MAGIC_far;           /* MAGIC */
  BYTE  EEOFFSET_TEST_NODE_ID_far;
  BYTE  EEOFFSET_TEST_POWER_LEVEL_far;
  BYTE  EEOFFSET_TEST_FRAME_COUNT_SUCCESS_far[2];
  BYTE  EEOFFSET_TEST_STATUS_far;
  BYTE  EEOFFSET_TEST_SOURCE_NODE_ID_far;
  BYTE  alarmStatus_far[MAX_NOTIFICATIONS];
  BYTE  EEOFFSET_MAGIC_far;           /* MAGIC */
  DWORD EEOFFSET_SLEEP_PERIOD_far;
  BYTE  EEOFFSET_MASTER_NODEID_far;
#ifdef SECURITY
  EEOFFS_NETWORK_SECURITY_STRUCT  EEOFFS_SECURITY;
#endif
} t_nvmApplDescriptor;

extern t_nvmApplDescriptor far nvmApplDescriptor;



/* Default values */
#define APPL_MAGIC_VALUE                 0x56

#endif /* _EEPROM_H_ */
