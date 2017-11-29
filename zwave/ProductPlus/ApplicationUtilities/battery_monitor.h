/**
 * @file
 * Implements functions that make it easier to support Battery monitor.
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _BATTERY_MONITOR_H_
#define _BATTERY_MONITOR_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/


/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * Battery levels
 */
typedef enum _BATT_LEVELS_
{
  BATT_DEAD_LEV = 0xff,
  BATT_LOW_LEV  = 0x00,
  BATT_HIGH_LEV = 0x10,
  BATT_FULL_LEV = 0x64
} BATT_LEVEL;


/**
 * Battery states
 */
typedef enum _ST_BATT_ {ST_BATT_FULL, ST_BATT_HIGH, ST_BATT_LOW, ST_BATT_DEAD} ST_BATT;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/**
 * @brief TimeToSendBattReport
 * Function description check if the battery level
 *
 * @return TRUE if battery report should be send, FALSE if battery level report
 *   should not be send
 */
BOOL TimeToSendBattReport( void );


/**
 * @brief TimeToSendBattReport
 * Send battery level report
 *
 * @param completedFunc callback function used to give the status of the transmition
 *  process
 * @return job status
 */
JOB_STATUS
SendBattReport(VOID_CALLBACKFUNC(completedFunc)(TRANSMISSION_RESULT * pTransmissionResult) );


/**
 * @brief InitBatteryMonitor
 * Init Battery module
 * @param wakeUpReason received from ApplicationInitHW()
 */
void InitBatteryMonitor(BYTE wakeUpReason);


/**
 * @brief SetLowBattReport
 * Reactivate Low battery report.
 */
void ActivateBattNotificationTrigger();


/**
 * @brief BatteryMonitorState
 * Get battery monitor state
 * @return Battry monitor state of type ST_BATT
 */
ST_BATT BatteryMonitorState(void);

#ifdef NOT_USED
/**
 * @brief  Read the Battery voltage level
 * @param[out] pointer to level battery level.
 * @return battery state is change since last measurement: TRUE if change else FALSE.
 */
BOOL
BatterySensorRead(BATT_LEVEL *battLvl );
#endif

#endif /* _BATTERY_MONITOR_H_ */
