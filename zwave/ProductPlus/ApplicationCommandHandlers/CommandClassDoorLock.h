/**
 * @file
 * Handler for Command Class Door Lock.
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMANDCLASSDOORLOCK_H_
#define _COMMANDCLASSDOORLOCK_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <CommandClass.h>
#include <agi.h>
/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * Returns the version of this CC.
 */
#define CommandClassDoorLockVersionGet() DOOR_LOCK_VERSION_V2

/**
 * Door Lock Mode (8 bit) will set the door lock device in unsecured or
 * secured mode as well as other peripheral settings.
 *
 * 1) Constant mode. Door will be unsecured until set back to secured mode by Command.
 * 2) Timeout mode. Fallback to secured mode after timeout has expired (set by Door Lock Configuration Set).
 * 3) This is Read Only State, i.e. Bolt is not fully retracted/engaged
 */
typedef enum
{
  DOOR_MODE_UNSEC = DOOR_LOCK_OPERATION_SET_DOOR_UNSECURED_V2,	/**< Door Unsecured 1)*/
  DOOR_MODE_UNSEC_TIMEOUT = DOOR_LOCK_OPERATION_SET_DOOR_UNSECURED_WITH_TIMEOUT_V2,	/**< Door Unsecured with timeout 2)*/
  DOOR_MODE_UNSEC_INSIDE = DOOR_LOCK_OPERATION_SET_DOOR_UNSECURED_FOR_INSIDE_DOOR_HANDLES_V2,	/**< Door Unsecured for inside Door Handles 1)*/
  DOOR_MODE_UNSEC_INSIDE_TIMEOUT = DOOR_LOCK_OPERATION_SET_DOOR_UNSECURED_FOR_INSIDE_DOOR_HANDLES_WITH_TIMEOUT_V2,	/**< Door Unsecured for inside Door Handles with timeout 2)*/
  DOOR_MODE_UNSEC_OUTSIDE = DOOR_LOCK_OPERATION_SET_DOOR_UNSECURED_FOR_OUTSIDE_DOOR_HANDLES_V2,	/**< Door Unsecured for outside Door Handles 1)*/
  DOOR_MODE_UNSEC_OUTSIDE_TIMEOUT = DOOR_LOCK_OPERATION_SET_DOOR_UNSECURED_FOR_OUTSIDE_DOOR_HANDLES_WITH_TIMEOUT_V2,	/**< Door Unsecured for outside Door Handles with timeout 2)*/
  DOOR_MODE_UNKNOWN = DOOR_LOCK_OPERATION_SET_DOOR_LOCK_STATE_UNKNOWN_V2, /**<	Door/Lock State Unknown 3). (Version 2)*/
  DOOR_MODE_SECURED = DOOR_LOCK_OPERATION_SET_DOOR_SECURED_V2	/**< Door Secured*/
} DOOR_MODE;


/**
 * Operation mode (1byte).
 * The Operation Type field can be set to either constant or timed operation. When
 * timed operation is set, the Lock Timer Minutes and Lock Timer Seconds fields
 * MUST be set to valid values.
 */
typedef enum
{
  DOOR_OPERATION_CONST = 0x01,   /**< Constant operation*/
  DOOR_OPERATION_TIMED = 0x02,   /**< Timed operation*/
  DOOR_OPERATION_RESERVED = 0x03 /**< 0X03– 0XFF  Reserved*/
} DOOR_OPERATION;



/**
 * Door Lock Operation Report data structure.
 * -----------------------------------------
 *
 * Inside/outside Door Handles Mode (4 bits).
 * These parameters indicate the activity of the door handles i.e. which
 * handle(s) has opened the door lock.
 * Bit| Outside Door Handles Mode (4 bits)         | Inside Door Handles Mode (4 bits)
 * ---|--------------------------------------------|-------------------------------------------
 * 0  | 0 = Handle 1 inactive; 1 = Handle 1 active | 0 = Handle 1 inactive; 1 = Handle 1 active
 * 1  | 0 = Handle 2 inactive; 1 = Handle 2 active | 0 = Handle 2 inactive; 1 = Handle 2 active
 * 2  | 0 = Handle 3 inactive; 1 = Handle 3 active | 0 = Handle 3 inactive; 1 = Handle 3 active
 * 3  | 0 = Handle 4 inactive; 1 = Handle 4 active | 0 = Handle 4 inactive; 1 = Handle 4 active
 *
 * Door condition (8 bits).
 * The Door Condition field indicates the hardware status of the door lock
 * device such as bolt and latch states.
 * Bit| Description
 *----|-----------------------------------
 * 0  | 0 = Door Open; 1 = Door Closed
 * 1  | 0 = Bolt Locked; 1 = Bolt Unlocked
 * 2  | 0 = Latch Open; 1 = Latch Closed
 * 3-7| Reserved
 */
typedef struct _CMD_CLASS_DOOR_LOCK_OPERATION_REPORT
{
  DOOR_MODE mode;
  BYTE insideDoorHandleMode : 4; /**< BIT-field Inside/outside Door Handles Mode (4 bits)*/
  BYTE outsideDoorHandleMode : 4; /**< BIT-field Inside/outside Door Handles Mode (4 bits)*/
  BYTE condition; /**< Door condition (8 bits)*/
  BYTE lockTimeoutMin; /**< Lock Timeout Minutes, valid values 1-254 decimal*/
  BYTE lockTimeoutSec; /**< Lock Timeout Seconds, valid 1-59 decimal*/
} CMD_CLASS_DOOR_LOCK_OPERATION_REPORT;

/**
 * Door Lock Configuration Set Command
 * -----------------------------------
 *
 * Inside/outside Door Handles State (4 bits).
 * The Door Handles field is to enable or disable the door handlers that are implemented in the
 * door lock device. For example there could be an inside as well as an outside door handler,
 * whereas the inside door handler can be handled by Z-Wave Commands and the outside door
 * handler will only unlock the door when successful authentication has been verified by e.g.
 * a keypad.
 * Bit| Outside Door Handles State (4 bits)         | Inside Door Handles State (4 bits)
 * ---|--------------------------------------------|-------------------------------------------
 * 0  | 0 = Handle 1 inactive; 1 = Handle 1 active | 0 = Handle 1 inactive; 1 = Handle 1 active
 * 1  | 0 = Handle 2 inactive; 1 = Handle 2 active | 0 = Handle 2 inactive; 1 = Handle 2 active
 * 2  | 0 = Handle 3 inactive; 1 = Handle 3 active | 0 = Handle 3 inactive; 1 = Handle 3 active
 * 3  | 0 = Handle 4 inactive; 1 = Handle 4 active | 0 = Handle 4 inactive; 1 = Handle 4 active
 */
typedef struct _CMD_CLASS_DOOR_LOCK_CONFIGURATION
{
  DOOR_OPERATION type;
  BYTE insideDoorHandleState : 4; /**< BIT-field Inside/outside Door Handles State (4 bits)*/
  BYTE outsideDoorHandleState : 4; /**< BIT-field Inside/outside Door Handles State (4 bits)*/
  BYTE lockTimeoutMin; /**< Lock Timeout Minutes, valid values 1-254 decimal*/
  BYTE lockTimeoutSec; /**< Lock Timeout Seconds, valid 1-59 decimal*/
} CMD_CLASS_DOOR_LOCK_CONFIGURATION;

/**
 * Cmd Class Door Lock data structure.
 * -----------------------------------
 *
 * Inside/outside Door Handles Mode (4 bits).
 * These parameters indicate the activity of the door handles i.e. which
 * handle(s) has opened the door lock.
 * Bit| Outside Door Handles Mode (4 bits)         | Inside Door Handles Mode (4 bits)
 * ---|--------------------------------------------|-------------------------------------------
 * 0  | 0 = Handle 1 inactive; 1 = Handle 1 active | 0 = Handle 1 inactive; 1 = Handle 1 active
 * 1  | 0 = Handle 2 inactive; 1 = Handle 2 active | 0 = Handle 2 inactive; 1 = Handle 2 active
 * 2  | 0 = Handle 3 inactive; 1 = Handle 3 active | 0 = Handle 3 inactive; 1 = Handle 3 active
 * 3  | 0 = Handle 4 inactive; 1 = Handle 4 active | 0 = Handle 4 inactive; 1 = Handle 4 active
 *
 * Inside/outside Door Handles State (4 bits).
 * The Door Handles field is to enable or disable the door handlers that are implemented in the
 * door lock device. For example there could be an inside as well as an outside door handler,
 * whereas the inside door handler can be handled by Z-Wave Commands and the outside door
 * handler will only unlock the door when successful authentication has been verified by e.g.
 * a keypad.
 * Bit| Outside Door Handles State (4 bits)         | Inside Door Handles State (4 bits)
 * ---|--------------------------------------------|-------------------------------------------
 * 0  | 0 = Handle 1 inactive; 1 = Handle 1 active | 0 = Handle 1 inactive; 1 = Handle 1 active
 * 1  | 0 = Handle 2 inactive; 1 = Handle 2 active | 0 = Handle 2 inactive; 1 = Handle 2 active
 * 2  | 0 = Handle 3 inactive; 1 = Handle 3 active | 0 = Handle 3 inactive; 1 = Handle 3 active
 * 3  | 0 = Handle 4 inactive; 1 = Handle 4 active | 0 = Handle 4 inactive; 1 = Handle 4 active
 *
 * Door condition (8 bits).
 * The Door Condition field indicates the hardware status of the door lock
 * device such as bolt and latch states.
 * Bit| Description
 *----|-----------------------------------
 * 0  | 0 = Door Open; 1 = Door Closed
 * 1  | 0 = Bolt Locked; 1 = Bolt Unlocked
 * 2  | 0 = Latch Open; 1 = Latch Closed
 * 3-7| Reserved
 */
typedef struct _CMD_CLASS_DOOR_LOCK_DATA
{
  DOOR_MODE mode;
  DOOR_OPERATION type;
  BYTE insideDoorHandleMode : 4; /**< BIT-field Inside Door Handles Mode (4 bits)*/
  BYTE outsideDoorHandleMode : 4; /**< BIT-field outside Door Handles Mode (4 bits)*/
  BYTE insideDoorHandleState : 4; /**< BIT-field Inside Door Handles State (4 bits)*/
  BYTE outsideDoorHandleState : 4; /**< BIT-field outside Door Handles State (4 bits)*/
  BYTE condition; /**< Door condition (8 bits)*/
  BYTE lockTimeoutMin; /**< Lock Timeout Minutes, valid values 1-254 decimal*/
  BYTE lockTimeoutSec; /**< Lock Timeout Seconds, valid 1-59 decimal*/
} CMD_CLASS_DOOR_LOCK_DATA;

/**
 * Disabling Lock timeout minutes and seconds.
 */
#define DOOR_LOCK_OPERATION_SET_TIMEOUT_NOT_SUPPORTED 0xFE

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/



/**
 * @brief handleCommandClassDoorLock
 * Handler for command class Doorlock
 * @param[in] rxOpt receive options of type RECEIVE_OPTIONS_TYPE_EX
 * @param[in] pCmd Payload from the received frame
 * @param[in] cmdLength Number of command bytes including the command
 * @return receive frame status.
 */
received_frame_status_t handleCommandClassDoorLock(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE cmdLength);



/**
 * @brief handleCommandClassDoorLockOperationSet
 * Comment function...
 * @param[in] mode door lock operation modes.
 * @return none
 */
extern void handleCommandClassDoorLockOperationSet( DOOR_MODE mode);


/**
 * @brief handleCommandClassDoorLockOperationReport
 * Comment function...
 * @param[out] pData data pointer of type CMD_CLASS_DOOR_LOCK_OPERATION_REPORT.
 * @return none
 */
extern void handleCommandClassDoorLockOperationReport(
  CMD_CLASS_DOOR_LOCK_OPERATION_REPORT* pData);

/**
 * @brief handleCommandClassDoorLockOperationSet
 * Comment function...
 * @param[in] pData data pointer of type CMD_CLASS_DOOR_LOCK_CONFIGURATION.
 * @return none
 */
extern void handleCommandClassDoorLockConfigurationSet(
  CMD_CLASS_DOOR_LOCK_CONFIGURATION* pData);


/**
 * @brief handleCommandClassDoorLockConfigurationReport
 * Comment function...
 * @param[out] pData data pointer of type CMD_CLASS_DOOR_LOCK_OPERATION.
 */
extern void handleCommandClassDoorLockConfigurationReport(
  CMD_CLASS_DOOR_LOCK_CONFIGURATION* pData);


/**
 * @brief Send DoorLock operation report.
 * @param[in] pProfile pointer to AGI profile
 * @param[in] sourceEndpoint source endpoint
 * @param[in] pData Pointer to door lock data.
 * @param[out] pCallback callback function pointer returning status destination node receive job.
 * @return status on the job.
 */
JOB_STATUS
CmdClassDoorLockOperationSupportReport(
  AGI_PROFILE* pProfile,
  BYTE sourceEndpoint,
  CMD_CLASS_DOOR_LOCK_OPERATION_REPORT* pData,
  VOID_CALLBACKFUNC(pCallback)(TRANSMISSION_RESULT * pTransmissionResult));

#endif /* _COMMANDCLASSDOORLOCK_H_ */
