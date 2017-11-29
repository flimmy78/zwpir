/**
 * @file
 * Handler for Command Class Wake Up.
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMANDCLASSWAKEUP_H_
#define _COMMANDCLASSWAKEUP_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <CommandClass.h>
#include <ZW_TransportEndpoint.h>
#include <ZW_classcmd.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * Returns the version of this CC.
 */
#define CmdClassWakeupVersionGet() WAKE_UP_VERSION_V2
#define CmdClassWakeupVersion() CmdClassWakeupVersionGet()

/**
 * Wakeup parameter types
 */
typedef enum _WAKEUP_PAR_
{
  WAKEUP_PAR_SLEEP_STEP,
  WAKEUP_PAR_MIN_SLEEP_TIME,
  WAKEUP_PAR_MAX_SLEEP_TIME,
  WAKEUP_PAR_DEFAULT_SLEEP_TIME,
  WAKEUP_PAR_COUNT
} WAKEUP_PAR;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/**
 * @brief Tell battery module that WakeUpNotification mode is active and sleep time should
 * be increased to 10 seconds.
 * @param[in] active parameter is used to active WakeUp-state. TRUE active and FALSE inactive.
 */
extern void
ZCB_WakeUpStateSet(
    BYTE active);


/**
 * @brief Transmits a Wake Up Notification command and handles callback.
 */
void
WakeUpNotification(void);

/**
 * @brief Transmits a Wake Up Notification command.
 * @param pCallback Pointer to callback function to be called upon transmission.
 * @return Status of the job.
 */
JOB_STATUS
CmdClassWakeupNotification(
    VOID_CALLBACKFUNC(pCallback)(TRANSMISSION_RESULT * pTransmissionResult));

/**
 * @brief Sets up the parameters defined in WAKEUP_PAR. The values are given in seconds.
 * @param[in] type Wake up parameter.
 * @param[in] time Time in seconds.
 */
void
SetWakeUpConfiguration(
    WAKEUP_PAR type,
    uint32_t time);

/**
 * @brief Handler for the Wake Up No More Information Command.
 */
extern void
handleWakeupNoMoreInfo(void);

/**
 * @brief Handler for Wake Up CC.
 * @param[in] rxOpt Receive options.
 * @param[in] pCmd Payload from the received frame.
 * @param[in] cmdLength Length of the given payload.
 * @return receive frame status.
 */
received_frame_status_t
HandleCommandClassWakeUp(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength);

/**
 * @brief Resets the saved node ID in NVM.
 */
void
CmdClassWakeUpNotificationMemorySetDefault(void);

/**
 * @brief Handler for Wake Up Interval Get Command.
 * @return The current wake up interval.
 */
extern uint32_t
handleWakeUpIntervalGet(void);

/**
 * @brief Set default sleep interval for device.
 * @param[in] sleep period for device to sleep.
 */
extern void
SetDefaultBatteryConfiguration(uint32_t sleep);

#endif /* _COMMANDCLASSWAKEUP_H_ */
