/***************************************************************************
*
* Copyright (c) 2001-2013
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Implements functions that make is easier to support
*              Battery monitor
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2013/06/25 14:44:03 $
*
****************************************************************************/
#ifndef _BATTERY_MONITOR_H_
#define _BATTERY_MONITOR_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/


/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

typedef enum _BATT_LEVELS_
{
  BATT_DEAD_LEV = 0xff,
  BATT_LOW_LEV  = 0x00,
  BATT_HIGH_LEV = 0x10,
  BATT_FULL_LEV = 0x64
} BATT_LEVEL;


typedef enum _ST_BATT_ {ST_BATT_FULL, ST_BATT_HIGH, ST_BATT_LOW, ST_BATT_DEAD} ST_BATT;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/*============================ TimeToSendBattReport ===============================
** Function description check if the battery level is low and send battery level report
**
**  txOption the RF tx option to use when sending battery level report
**  completedFunc callback function used to give the status of the transmition 
**  process
**
** Return TRUE if battery report should be send, FALSE if battery level report 
**        should not be send
**
** Side effects: 
**
**-------------------------------------------------------------------------*/
BOOL
TimeToSendBattReport(BYTE txOption,
                    VOID_CALLBACKFUNC(completedFunc)(BYTE) );
/*============================ InitBatteryMonitor ===============================
** Function description
** Init Battery module
**
** Side effects: 
**
**-------------------------------------------------------------------------*/
void
InitBatteryMonitor(BYTE wakeUpReason);


/*============================ SetLowBattReport ===============================
** Function description
** Set status if Lowbatt reoprt should be active og deactive. FALSE is deactive
**
** Side effects: 
**
**-------------------------------------------------------------------------*/
void
SetLowBattReport(BOOL status);



/** 
 * @brief BatteryMonitorState
 * Comment function...
 * @return Battry monitor state of type ST_BATT
 */
ST_BATT BatteryMonitorState(void);

#endif /* _BATTERY_MONITOR_H_ */
