/***************************************************************************
*
 * @author Christian Salmony Olsen
* Copyright (c) 2001-2013
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Implements functions that make is easier to support
*              Battery Operated Nodes
*        All far variables (NVM offsets) should be defined in the application's eeprom.c module
*        in the struct t_nvmApplDescriptor
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2013/06/25 14:44:03 $
*
****************************************************************************/
#ifndef _BATTERY_PLUS_H_
#define _BATTERY_PLUS_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <CommandClassWakeUp.h>
#include <ZW_typedefs.h>
#include <ZW_basis_api.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * Seconds in minutes
 */
#define SECONDS_IN_MINUTE    (DWORD)60

/**
 * Seconds in hours
 */
#define SECONDS_IN_HOUR      (DWORD)(60 * SECONDS_IN_MINUTE)

/**
 * Seconds in day
 */
#define SECONDS_IN_DAY       (DWORD)(24 * SECONDS_IN_HOUR)

/**
 * FLIRS device TX-option macro
 */
#define FLIRS_DEVICE_OPTIONS_MASK_MODIFY(deviceOptionsMask) \
  deviceOptionsMask = (deviceOptionsMask & ~(APPLICATION_NODEINFO_LISTENING)) \
    | APPLICATION_NODEINFO_NOT_LISTENING | APPLICATION_FREQ_LISTENING_MODE_1000ms

/**
 * Battery mode
 */
typedef enum _BATT_MODE_
{
  BATT_MODE_NOT_LISTENING = APPLICATION_NODEINFO_NOT_LISTENING,
  BATT_MODE_LISTENING = APPLICATION_NODEINFO_LISTENING
} BATT_MODE;


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/**
 * @brief SetDefaultBatteryConfiguration
 * Set default sleep periode for device.
 * @param sleep period for device to sleep.
 */
void
SetDefaultBatteryConfiguration(DWORD sleep);

/**
 * @brief LoadBatteryConfiguration
 * Load battery parameters from NVM.
 */
void
LoadBatteryConfiguration(void);


/**
 * @brief Count down wakeUp counter and call PowerDownNow() if going to sleep.
 * @param[in] mode listening or not listening
 * @param[in] wakeUpReason is ApplicationInitHW(..) wake up reason.
 * @return WakeUp status, TRUE => wake up, FALSE => sleep.
 */
BOOL
BatteryInit(
    BATT_MODE mode,
    BYTE wakeUpReason);

/**
 * @brief Starts the power down timer. When it expires and there's no tasks running, the device is
 * powered down. The timer is only started if it is not already running.
 */
void
ZCB_StartPowerDownTimer(void);

/**
 * @brief Stop powerDown timer.
 */
void
ZCB_StopPowerDownTimer(void);

/**
 * @brief When this function is called, it's time to power down the sensor.
 */
void
PowerDownNow(void);

/**
 * @brief Sets the power down timeout value.
 * @param[in] Timeout value [0;255] represents [0;25500] ms in steps of 100 ms.
 */
void
ZCB_SetPowerDownTimeout(
    BYTE timeout);

/**
 * @brief This function set the powerdown timer delay time. The timer resolution
 * is 1 sec. The value specifiy the time the device will stay wake before it
 * goes to powerdown state. Timeout is minimum 10 seconds if Command Class
 * WakeUp State is active.
 * @param[in] timeout parameter in seconds
 */
void
ZCB_SetPowerDownTimeoutWakeUpStateCheck(
    BYTE timeout);


/**
 * @brief WakeUpStateSet
 * Tell battery module that WakeUpNotification mode is active and sleep time should
 * be increased to 10 seconds.
 * @param active parameter is used to active WakeUp-state. TRUE active and FALSE inactive.
 */
void ZCB_WakeUpStateSet(BYTE active);

/**
 * @brief Called to check whether the application is ready to power down. Must be implemented by
 * the application.
 * @return TRUE if ready to power down, FALSE otherwise.
 */
extern BYTE
AppPowerDownReady(void);

/**
 * @brief Handler for Wake Up Interval Get Command.
 * @return The current wake up interval.
 */
DWORD
handleWakeUpIntervalGet(void);


#endif /* _BATTERY_PLUS_H_ */

