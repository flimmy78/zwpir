/**
 * @file
 * Handler for Command Class User Code.
 *
 * The purpose of the User Code Command Class is to supply a enabled
 * Door Lock Device with a command class to manage user codes.
 *
 * User Identifier (8 bits).
 * -------------------------
 * The User Identifier used to recognise the user identity. The User Identifier
 * values MUST be a sequence starting from 1. This field can be ignored in case
 * the node only supports one User Code. Setting the User Identifier to 0 will
 * address all User Identifiers available in the device.
 *
 * USER_CODE1, USER_CODEn.
 * -----------------------
 * These fields contain the user code. Minimum code length is 4 and maximum 10
 * ASCII digits. The number of data fields transmitted can be determined from
 * the length field returned by the ApplicationCommandHandler. The user code
 * fields MUST be initialize to 0x00 0x00 0x00 0x00 (4 bytes) when User ID
 * Status is equal to 0x00.
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMANDCLASSUSERCODE_H_
#define _COMMANDCLASSUSERCODE_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_tx_mutex.h>
/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * Returns the version of this CC.
 */
#define CommandClassUserCodeVersionGet() USER_CODE_VERSION

/**
 * User ID Status.
 * ---------------
 * The User ID Status field indicates the state of the User Identifier. All 
 * other values not mentioned in below list are reserved for future 
 * implementation.
 * Hex | Description
 * ----|---------------------------
 *  00 | Available (not set)
 *  01 | Occupied
 *  02 | Reserved by administrator
 *  FE | Status not available
 */
typedef enum 
{
  USER_ID_AVAILBLE = 0x00, /**< Available (not set)*/
  USER_ID_OCCUPIED = 0x01, /**< Occupied*/
  USER_ID_RESERVED = 0x02, /**< Reserved by administrator*/
  USER_ID_STATUS_COUNT,
  USER_ID_NO_STATUS = 0xFE /**<	Status not available*/
} USER_ID_STATUS;




/**
 * User Code Set Command (SDS11060.doc) define max 10 ASCII digts
 */
#define USERCODE_MIN_LEN 4
#define USERCODE_MAX_LEN 10


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/** 
 * @brief handleCommandClassUserCodeSet
 * The User Code Set Command used to set a User Code in the device.
 * @param identifier User Identifier. 
 * @param id user Id status.
 * @param pUserCode pointer to UserCode data.
 * @param len UserCode data.
 * @return none.
 */
extern BOOL
handleCommandClassUserCodeSet(
  BYTE identifier,
  USER_ID_STATUS id,
  BYTE* pUserCode,
  BYTE len);

/** 
 * @brief handleCommandClassUserCodeIdGet
 * The User Code Get ID.
 * @param identifier User Identifier.
 * @param pId pointer to return Id.
 * @return status valid boolean.
 */
extern BOOL
handleCommandClassUserCodeIdGet(
  BYTE identifier,
  USER_ID_STATUS* pId);


/** 
 * @brief handleCommandClassUserCodeDataReport
 * The User Code Report Command can be used by e.g. a door lock device to send a 
 * report either unsolicited or requested by the User Code Get Command.
 * @param identifier User Identifier. 
 * @param pUserCode pointer to UserCode data.
 * @param len UserCode data.
 * @return status valid boolean.
 */
extern BOOL
handleCommandClassUserCodeReport(
  BYTE identifier,
  BYTE* pUserCode,
  BYTE *pLen);


/** 
 * @brief handleCommandClassUserCodeUsersNumberReport
 * The Users Number Report Command used to report the maximum number of USER CODES 
 * the given node supports. The Users Number Report Command can be send requested
 * by the Users Number Get Command.
 * @return maximum number of USER CODES.
 */
extern BYTE
handleCommandClassUserCodeUsersNumberReport(
  void);

/** 
 * @brief handleCommandClassUserCode
 * @param option IN Frame header info.
 * @param sourceNode IN Command sender Node ID.
 * @param pCmd IN Payload from the received frame, the union should be used to access 
 * the fields.
 * @param cmdLength IN Number of command bytes including the command.
 * @return none.
 */
void
handleCommandClassUserCode(
  BYTE  option,                 
  BYTE  sourceNode,              
  ZW_APPLICATION_TX_BUFFER *pCmd,
  BYTE   cmdLength);               


/** 
 * @brief CmdClassUserCodeSupportReport
 * Comment function...
 * @param par description..
 * @return enum JOB_STATUS
 */
JOB_STATUS
CmdClassUserCodeSupportReport(
  BYTE destNode, 
  BYTE userIdentifier,
  BYTE userIdStatus,
  BYTE* pUserCode,
  BYTE userCodeLen,
  VOID_CALLBACKFUNC(pCbFunc)(BYTE val));

#endif /* _COMMANDCLASSUSERCODE_H_ */
