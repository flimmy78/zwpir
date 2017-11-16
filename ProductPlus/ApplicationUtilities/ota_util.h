/**
 *
 * Copyright (c) 2001-2013
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 * @file ota_util.h
 *
 * @brief Header file for ota_util.c. This module implements
 *        functions used in combination with command class firmware update
 *
 *
 *
 *
 * Author: Samer Seoud
 *
 * Last Changed By: $Author: tro $
 * Revision: $Revision: 0.00 $
 * Last Changed: $Date: 2013/06/21 10:18:01 $
 *
 */

#ifndef _OTA_UTIL_H_
#define _OTA_UTIL_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/


/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * enum type OTA_STATUS use to 
 */
typedef enum _OTA_STATUS_
{
  OTA_STATUS_DONE = 0,
  OTA_STATUS_ABORT = 1,
  OTA_STATUS_TIMEOUT = 2
} OTA_STATUS;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/



/** 
 * @brief OtaInit
 * Initialization of Firmware Update module "OTA" has  3 iput paramers txOption, 
 * pOtaStart and pOtaFinish. 
 * txOption is used to set options on funtion calls on module CommandClassFirmwareUpdate. 
 * Remember also to control transmit option in function HandleCommandClassFWUpdate(..).
 * Input parameters pOtaStart and pOtaFinish is used to inform Application of 
 * the status of firmware update and give application possibility to control 
 * start of firmware update. It is possible to not call OtaInit and the process 
 * run without the application with standard paramers for txOption.
 *
 * @param txOption is used to set transmit options for unsolicited events as ex.
 *  bit parameters TRANSMIT_OPTION_RETURN_ROUTE | TRANSMIT_OPTION_ACK. 
 * @param pOtaStart function pointer is called when firmware update is ready to 
 * start. It is up to the application allow or reject the process by returning 
 * value in the call. Can be set to NULL.
 * @param pOtaFinish function pointer is called when the firmware update proces 
 * i finish. As input parameter is status of the process of type OTA_STATUS. 
 * If set to NULL, ota_util module will reboot when process is finish.
 */
void
OtaInit( 
  BYTE txOption,
  BOOL (CODE *pOtaStart)(void), 
  VOID_CALLBACKFUNC(pOtaFinish)(BYTE val));


#endif /* _OTA_UTIL_H_ */

