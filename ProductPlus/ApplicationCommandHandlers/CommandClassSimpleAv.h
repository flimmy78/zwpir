/**
 * @file
 * Handler for Command Class Simple AV.
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMANDCLASSSIMPLEAV_H_
#define _COMMANDCLASSSIMPLEAV_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_tx_mutex.h>

#define MUTE                  0x0001
#define BT_0                  0x0006
#define BT_1                  0x0007
#define BT_2                  0x0008

#define BT_3                  0x0009
#define BT_4                  0x000A
#define BT_5                  0x000B
#define BT_6                  0x000C
#define BT_7                  0x000D
#define BT_8                  0x000E
#define BT_9                  0x000F

#define PLAY                  0x0013
#define STOP                  0x0014
#define PAUSE                 0x0015
#define FWD                   0x0016
#define REV                   0x0017

#define MENU                  0x001D
#define UP                    0x001E
#define DOWN                  0x001F
#define LEFT                  0x0020
#define RIGHT                 0x0021
#define PAGEUP                0x0022
#define PAGE_DOWN             0x0023
#define ENTER                 0x0024
#define ON_OFF                0x0027

#define ANGLE                 0x003C
#define AUDIO                 0x0041
#define RETURN                0x004B
#define DELETE                0x007D
#define USB_DVDROM_EJECT      0x0091
#define BLUE                  0x009A
#define GREEN                 0x009B
#define RED                   0x009D
#define YELLOW                0x009F
#define HOME                  0x00AF
#define REPEAT                0x0107
#define SETUP                 0x0115
#define NEXT                  0x011B
#define PREV                  0x011C
#define SLOW                  0x011E
#define SUBTITLE              0x0130
#define TITLE                 0x0156
#define ZOOM                  0x0169
#define INFO                  0x017A
#define CAPS_NUM              0x017B
#define TV_MODE               0x017C
#define SOURCE                0x017D
#define FILE_MODE             0x017E
#define TIME_SEEK             0x017F
#define SUSPEND               0x0194

#define NO_KEY                0xFFFF
#define NOT_BTN               0x0000

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

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




/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/



/** 
 * @brief handleCommandClassSimpleAv
 * @param option IN Frame header info.
 * @param sourceNode IN Command sender Node ID.
 * @param pCmd IN Payload from the received frame, the union should be used to access 
 * the fields.
 * @param cmdLength IN Number of command bytes including the command.
 * @return none.
 */
extern void
handleCommandClassSimpleAv(
  BYTE  option,           
  BYTE  sourceNode,        
  ZW_APPLICATION_TX_BUFFER *pCmd, 
  BYTE   cmdLength);
  
/** 
 * @brief getApplSimpleAvSupported
 * Get the supported AV commands bitmask bytes assigned to report number reportNo
 * The supported AV commands 
 * @param reportNo IN the report number of the AV commands bitmask bytes
 * @param avCmdBitMask OUT the generated AV commands bitmask bytes 
 * @return length of the supported AV cmd report
 * @return None.
 */
extern BYTE 
getApplSimpleAvSupported (BYTE reportNo,
                         BYTE *avCmdBitMask);

/** 
 * @brief getApplSimpleAvReports
 * Get the supported AV commands bitmask bytes reports number
 * The supported AV commands is reported as a bit mask where bit 0 in bytes 1 is 1 if AV cmd#1
 * is supported else its false. The bitmask always start from AV cmd# 1 to latest supported AV cmd#
 * the bit mask and be devided over several reports.
 * @return number of the AV commands bitmask bytes reports
  */
extern BYTE 
getApplSimpleAvReports ();


/** 
 * @brief CmdClassSimpleAvSet
 * Sent a somple AV command to a destination node
 * @param option: tx options
 * @param dstNode: destination node
 * @param bCommand: AV command
 * @param bKeyAttrib: AV command attribute
 * @param pCbFunc: call back function
 * @return JOB status
 * @return None.
 */
extern JOB_STATUS
CmdClassSimpleAvSet(
  BYTE option,
  BYTE dstNode,
  WORD bCommand,                  /* IN What to do*/
  BYTE bKeyAttrib,                /*Key attribute*/
  VOID_CALLBACKFUNC(pCbFunc)(BYTE val));
#endif /*  _COMMANDCLASSSIMPLEAV_H_ */
