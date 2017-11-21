/**
*
* Copyright (c) 2001-2014
* Sigma Designs, Inc.
* All Rights Reserved
*
* @file appl_timer_api.h
*
* @brief Timer service functions that handle delayed functions calls.
*        The time resoltuion is 1 second.
*
* Author: Samer Seoud
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2014/09/25 10:13:28 $
*
*/

#ifndef _APPL_TIMER_API_H_
#define _APPL_TIMER_API_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/


/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * The maximum number of application timers that can be created.
 */
#define APPL_TIMER_MAX   10

/**
 * Start an application timer that runs once.
 */
#define APPL_TIMER_ONETIME     0
/**
 * Start an application timer that runs forever.
 */
#define APPL_TIMER_FOREVER      (BYTE)-1


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/**
 * @brief ApplTimerGetTime
 * Get the time passed since the start of the application timer
 * It return the time (in seconds) passed since the timer is created or restarted
 * The timer is identified with the timer handle
 * @param btimerHandle IN the application timer ID.
 * @return if it is less than 0xFFFFFFFF, then it is the number of seconds passed since the timer was
 *         created or restarted. If it equal 0xFFFFFFFF then the timer handle is invalid.
 */
extern DWORD
ApplTimerGetTime(BYTE btimerHandle);

/**
 * @brief ApplTimerStart
 * Creats an application timer instance
 * @param func IN Timeout function address.
 * @param ltimerTicks IN Timeout value (in seconds).
 * @param brepeats IN   Number of function calls if function func before the timer stop (-1: forever).
 * @return Returns timer handle ID creatd (1 to APPL_TIMER_MAX), 0 if no timer is created.
 */
extern BYTE
ApplTimerStart(
  VOID_CALLBACKFUNC(func)(void),
  DWORD ltimerTicks,
  BYTE brepeats);


/**
 * @brief ApplTimerRestart
 * Set the specified timer back to the initial value.
 * @param timerHandle IN The timer ID.
 * @return Returns TRUE if timer restarted, FALSE if timer ID is invalid or timer is not running
 */
extern  BYTE
ApplTimerRestart(
BYTE timerHandle);

/**
 * @brief ApplTimerStop
 * Stop the specified timer. Set the timerHandle to 0
 * @param pbTimerHandle IN pointer to the timer Handle.
 * @return none.
 */
extern void
ApplTimerStop(
  BYTE *pbTimerHandle);

/**
 * @brief ApplTimerInit
 * Initalize the application timer subsystem. Must only be called once
 * @return TRUE if application timer system initaliazed correctly else FALSE.
 */
extern BOOL
ApplTimerInit();
#endif /* _SLOW_TIMER_API_H_ */
