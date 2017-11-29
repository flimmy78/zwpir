/****************************************************************************
 *
 * Copyright (c) 2001-2013
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: This header file contains defines for application version
 *  in a generalized way.
 *
 *  Don't change the name of the file, and son't change the names of
 *  APP_VERSION and APP_REVISION, as they are handled automatically by
 *  the release procedure. The version information will be set automatically
 *  by the "make_release.bat"-script.
 *
 * Author:   Erik Friis Harck
 *
 * Last Changed By:  $Author: efh $
 * Revision:         $Revision: 11456 $
 * Last Changed:     $Date: 2008-09-25 16:29:18 +0200 (Thu, 25 Sep 2008) $
 *
 ****************************************************************************/
#ifndef _CONFIG_APP_H_
#define _CONFIG_APP_H_

#ifdef __C51__
#include "ZW_product_id_enum.h"
#include <commandClassManufacturerSpecific.h>
#endif

/****************************************************************************
 *
 * Application version, which is generated during release of SDK.
 * The application developer can freely alter the version numbers
 * according to his needs.
 *
 ****************************************************************************/
#define APP_VERSION 1
#define APP_REVISION 95

/****************************************************************************
 *
 * Defines device generic and specific types
 *
 ****************************************************************************/
#define GENERIC_TYPE GENERIC_TYPE_SENSOR_NOTIFICATION
#define SPECIFIC_TYPE SPECIFIC_TYPE_NOTIFICATION_SENSOR
/**
 * See ZW_basic_api.h for ApplicationNodeInformation field deviceOptionMask
 */
#define DEVICE_OPTIONS_MASK   APPLICATION_NODEINFO_NOT_LISTENING | APPLICATION_NODEINFO_OPTIONAL_FUNCTIONALITY

/****************************************************************************
 * Defines used to initialize the Z-Wave Plus Info Command Class.
 *
 ****************************************************************************/
#define APP_ROLE_TYPE ZWAVEPLUS_INFO_REPORT_ROLE_TYPE_SLAVE_SLEEPING_REPORTING
#define APP_NODE_TYPE ZWAVEPLUS_INFO_REPORT_NODE_TYPE_ZWAVEPLUS_NODE
#define APP_ICON_TYPE ICON_TYPE_SPECIFIC_SENSOR_NOTIFICATION_HOME_SECURITY
#define APP_USER_ICON_TYPE ICON_TYPE_SPECIFIC_SENSOR_NOTIFICATION_HOME_SECURITY

/****************************************************************************
 *
 * Defines used to initialize the Manufacturer Specific Command Class.
 *
 ****************************************************************************/
#define APP_MANUFACTURER_ID     MFG_ID_SIGMA_DESIGNS

#define APP_PRODUCT_TYPE_ID     PRODUCT_TYPE_ID_ZWAVE_PLUS
#define APP_PRODUCT_ID          PRODUCT_ID_SensorPIR

#define APP_FIRMWARE_ID         APP_PRODUCT_ID | (APP_PRODUCT_TYPE_ID << 8)

#define APP_DEVICE_ID_TYPE      DEVICE_ID_TYPE_PSEUDO_RANDOM
#define APP_DEVICE_ID_FORMAT    DEVICE_ID_FORMAT_BIN

/****************************************************************************
 *
 * Defines used to initialize the Association Group Information (AGI)
 * Command Class.
 *
 ****************************************************************************/
#define APP_NUMBER_OF_ENDPOINTS 1
#define MAX_ASSOCIATION_GROUPS      4
#define MAX_ASSOCIATION_IN_GROUP    5

#define AGITABLE_LIFELINE_GROUP \
  {COMMAND_CLASS_BATTERY, BATTERY_REPORT}, \
  {COMMAND_CLASS_NOTIFICATION_V3, NOTIFICATION_REPORT_V3}, \
  {COMMAND_CLASS_DEVICE_RESET_LOCALLY, DEVICE_RESET_LOCALLY_NOTIFICATION}

#define  AGITABLE_ROOTDEVICE_GROUPS \
 {ASSOCIATION_GROUP_INFO_REPORT_PROFILE_NOTIFICATION, NOTIFICATION_REPORT_BURGLAR_V3, {COMMAND_CLASS_BASIC,BASIC_SET},"Basic set"}, \
 {ASSOCIATION_GROUP_INFO_REPORT_PROFILE_NOTIFICATION, NOTIFICATION_REPORT_BURGLAR_V3, {COMMAND_CLASS_NOTIFICATION_V3, NOTIFICATION_REPORT_V3},"Notification report"}


/**
 * PowerDownTimeout determines the number of seconds before powerdown.
 */
#define MSEC_200_POWERDOWNTIMEOUT     2  /*200 mSec*/
#define SEC_2_POWERDOWNTIMEOUT       20
#define SEC_10_POWERDOWNTIMEOUT     100
#define LEARNMODE_POWERDOWNTIMEOUT  255


/**
 * Max notifications types
 */
#define MAX_NOTIFICATIONS 1

#endif /* _CONFIG_APP_H_ */

