/**
 * @file
 * Handling of power functionality and helper module for the Wake Up CC.
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */
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
 * PowerDownTimeout determines the number of seconds the sensor is kept alive
 * between powerdowns. The default is one second, which is probably too little
 * if you are routing in your network. ZWave+ defined: go to sleep > 10 SDS11846-2.doc
 */
#define DEFAULT_POWERDOWNTIMEOUT    11

/**
 * KeepAliveTimeout determines the number of seconds the sensor is kept alive
 * when the button is activated for more than KEEPALIVEACTIVATEPERIOD seconds.
 * This can be used when installing the sensor in a network. Default keepalive
 * is 30 seconds.
 */
#define DEFAULT_KEEPALIVETIMEOUT   30

/**
 * Press and hold button for this period of time to enter keepalive mode
 * Default is 3 seconds.
 */
#define DEFAULT_KEEPALIVEACTIVATETIMEOUT  3

/**
 * WAKEUPCOUNT holds the number of times WUT has been activated. The value is stored
 * in EEPROM and is used to determine when to send a Wakeup Information frame.
 * Default is 5 which means that when the sensor has been woken 5 times it will send
 * a Wakeup Information frame.
 */
#define DEFAULT_WAKEUPCOUNT 5

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
 * @brief Loads battery parameters from NVM.
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
 * @param[in] timeout value [0;255] represents [0;25500] ms in steps of 100 ms.
 */
void
ZCB_SetPowerDownTimeout(
    BYTE timeout);

/**
 * @brief Sets the power down timeout value and verify CommandClass Wake-up is active. If WakeUp is active
 * the minimum timeout is 10 seconds!
 * @param[in]  timeout value [0;255] represents [0;25500] ms in steps of 100 ms.
 */
void
ZCB_SetPowerDownTimeoutWakeUpStateCheck(
    BYTE timeout);

/**
 * @brief Called to check whether the application is ready to power down. Must be implemented by
 * the application.
 * @return TRUE if ready to power down, FALSE otherwise.
 */
extern BYTE
AppPowerDownReady(void);

#endif /* _BATTERY_PLUS_H_ */

