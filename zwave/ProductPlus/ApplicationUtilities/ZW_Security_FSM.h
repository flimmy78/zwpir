/*************************************************************************** 
* 
* Copyright (c) 2013
* Sigma Designs, Inc. 
* All Rights Reserved 
* 
*--------------------------------------------------------------------------- 
* 
* Description:
*   Finite state machine (FSM) for the security engine. The primary purpose
*   is to "smooth" the frame flow and avoid deadlocks in long communication
*   sessions (e.g. firmware updates).
*
*   The transition table can be found in the .c file of the same name
*   (ZW_Security_FSM.c).
* 
* Author:   Jakob Buron 
* 
* Last Changed By:  $Author: jbu $ 
* Revision:         $Revision: 27690 $ 
* Last Changed:     $Date: 2014-01-06 10:39:31 +0100 (ma, 06 jan 2014) $ 
* 
****************************************************************************/
#ifndef ZW_SECURITY_FSM_H_
#define ZW_SECURITY_FSM_H_

/* States */
typedef enum SECURITY_FSM_STATES {
  ST_ANY = 0xFF,
  ST_UNCHANGED = 0xFE,            /* Dont change state. Only valid as next state */
  ST_IDLE = 0,
  ST_SENDING_NONCE_GET = 1,
  ST_AWAITING_NONCE_REPORT = 2,
  ST_SENDING_ENCAP = 3,
  ST_SENDING_NONCE_REPORT = 4,
  ST_AWAITING_ENCAP = 5,
} SECURITY_FSM_STATE_T;

/* Events */
typedef enum SECURITY_FSM_EV {
  EV_ANY = 0xFF,
  EV_NONCE_GET_ARRIVES = 0,
  EV_RETURN_TO_IDLE = 1, /* Many timeout and Fail return code events have been collapsed into this */
  EV_NONCE_GET_TRANSMITTING = 7,
  EV_NONCE_GET_TRANSMIT_OK = 2,
  EV_NONCE_REPORT_ARRIVES = 3,
  EV_NONCE_REPORT_TRANSMITTING = 8,
  EV_NONCE_REPORT_TRANSMIT_OK = 4,
  EV_SEC_ENCAP_ARRIVES = 5,
  EV_SEC_ENCAP_TRANSMITTED = 6,
} SECURITY_FSM_EV_T;


/*=========================   InitSecurityFsm   =============================
**
**  Initializes the finitie state machine. Must be called before
**  posting events and getting state.
**
**  Side effects: None
**
**-------------------------------------------------------------------------*/
void InitSecurityFsm();

/*============================   PostEvent   ===============================
**
**  Posts an event to the finite state machine FSM.
**
**  Side effects: Advances the FSM to the next state.
**
**-------------------------------------------------------------------------*/
void PostEvent(SECURITY_FSM_EV_T ev);

/*=======================   GetSecurityFsmState   ==========================
**
**  Return the current state of the FSM.
**
**  The security module uses this function to decide if a particular action
**  can be allowed at the current time.
**
**  Side effects: None.
**
**-------------------------------------------------------------------------*/
SECURITY_FSM_STATE_T GetSecurityFsmState();

#endif /* ZW_SECURITY_FSM_H_ */
