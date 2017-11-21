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
#include <ZW_typedefs.h>
#include <ZW_tx_mutex.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * Returns the version of this CC.
 */
#define CmdClassWakeupVersion() WAKE_UP_VERSION_V2

/**
 * Wakeup parameter types
 */
typedef enum _WAKEUP_PAR_
{
  WAKEUP_PAR_SLEEP_STEP,
  WAKEUP_PAR_MIN_SLEEP_TIME,
  WAKEUP_PAR_MAX_SLEEP_TIME,
  WAKEUP_PAR_DEFAULT_SLEEP_TIME,
  WAKEUP_PAR_MAX
} WAKEUP_PAR;

/**
 * State Wait
 */
#define WAKEUP_STATE_WAIT    1

/**
 * State done
 */
#define WAKEUP_STATE_DONE    2

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

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
 * @brief WakeUpNotification
 * Function sends off the Wakeup notification command
 */
void
WakeUpNotification(void);


/**
 * @brief BM_SetWakeUpConfiguration
 *  Function set the wakeup timing configuration values.
 * Sleep step time, default sleep time, max sleep time, and
 * min sleep time. these values are in seconds
 *  these values are constants
 * @param *config pointer to the wakeup configuration data structure
 */
void SetWakeUpConfiguration(WAKEUP_PAR type, XDWORD time);


/**
 * @brief handleWakeUpNoMoreInfo
 * Get frame WAKE_UP_NO_MORE_INFORMATION_V2.
 */
extern void
handleWakeupNoMoreInfo(void);
/**
 * @brief WakeUpNotification
 * @param pCbFunc function pointer for the result of the job
 * @return JOB_STATUS
 */
JOB_STATUS
CmdClassWakeupNotification(
  VOID_CALLBACKFUNC(pCbFunc)(BYTE val));

/**
 * @brief HandleCommandClassWakeUp
 * @param option IN Frame header info.
 * @param sourceNode IN Command sender Node ID.
 * @param pCmd IN Payload from the received frame, the union should be used to access
 * the fields.
 * @param cmdLength IN Number of command bytes including the command.
 * @return none.
 */
void
HandleCommandClassWakeUp(
  BYTE  option,                    /* IN Frame header info */
  BYTE  sourceNode,                /* IN Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER *pCmd,  /* IN Payload from the received frame, the union */
  /*    should be used to access the fields */
  BYTE   cmdLength                 /* IN Number of command bytes including the command */
);

/**
 * @brief CmdClassWakeUpNotificationMemorySetDefault
 * Set memory EEOFFSET_MASTER_NODEID_far to 0 (deafult value).
 */
void CmdClassWakeUpNotificationMemorySetDefault(void);

/**
 * @brief Handler for Wake Up Interval Get Command.
 * @return The current wake up interval.
 */
extern DWORD
handleWakeUpIntervalGet(void);
#endif /* _COMMANDCLASSWAKEUP_H_ */
