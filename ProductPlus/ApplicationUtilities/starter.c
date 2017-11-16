/*********************************  starter.c  ******************************
 *           #######
 *           ##  ##
 *           #  ##    ####   #####    #####  ##  ##   #####
 *             ##    ##  ##  ##  ##  ##      ##  ##  ##
 *            ##  #  ######  ##  ##   ####   ##  ##   ####
 *           ##  ##  ##      ##  ##      ##   #####      ##
 *          #######   ####   ##  ##  #####       ##  #####
 *                                           #####
 *          Z-Wave, the wireless language.
 *
 *              Copyright (c) 2001
 *              Zensys A/S
 *              Denmark
 *
 *              All Rights Reserved
 *
 *    This source file is subject to the terms and conditions of the
 *    Zensys Software License Agreement which restricts the manner
 *    in which it may be used.
 *
 *---------------------------------------------------------------------------
 * Copyright Zensys A/S, 2001
 *
 * Description: empty application for evaluation kit.
 *
 * Author:   Peter Shorty, Erik Friis Harck
 *
 * Last Changed By:  $Author: psh $
 * Revision:         $Revision: 17420 $
 * Last Changed:     $Date: 2010-04-16 11:02:50 +0200 (Fri, 16 Apr 2010) $
 *
 ****************************************************************************/
#ifdef PATCH_ENABLE
/****************************************************************************/
/* Include assembly MACRO definitions for patch insertions.                 */
/*                                                                          */
/* Define $SET (MAKE_PATCHABLE_CODE) for making patchable code destinned    */
/* for OTP or ROM memory.                                                   */
/* Undefine $RESET (MAKE_PATCHABLE_CODE) for making code containing patch   */
/* code destinned for RAM or FLASH memory.                                  */
/****************************************************************************/
#if defined(WORK_PATCH) || defined(STARTER_PATCH)
/* Making code containing patch code destinned for development RAM memory.  */
#pragma asm
$RESET (MAKE_PATCHABLE_CODE)
$INCLUDE (ZW_patch.inc)
#pragma endasm
/* Rename CODE class to CODE_PATCH */
#pragma userclass (code = PATCH)
/* Rename CONST class to CONST_PATCH */
#pragma userclass (const = PATCH)
/* Rename XDATA class to XDATA_PATCH */
#pragma userclass (xdata = PATCH)
#else
/* Making patchable code destinned for OTP or ROM memory.                   */
#pragma asm
$SET (MAKE_PATCHABLE_CODE)
$INCLUDE (ZW_patch.inc)
#pragma endasm
#endif /* elsif defined(WORK_PATCH) || defined(STARTER_PATCH) */
#endif /* PATCH_ENABLE */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "config_app.h"
#include <ZW_patch.h>
#ifdef ZW_CONTROLLER
#ifdef ZW_INSTALLER
#include <ZW_controller_installer_api.h>
#else
#ifdef ZW_CONTROLLER_STATIC
#ifdef ZW_CONTROLLER_BRIDGE
#include <ZW_controller_bridge_api.h>
#else
#include <ZW_controller_static_api.h>
#endif /* ZW_CONTROLLER_BRIDGE */
#else
#include <ZW_controller_api.h>
#endif /* ZW_CONTROLLER_STATIC */
#endif /* ZW_INSTALLER */
#endif /* ZW_CONTROLLER */

#ifdef ZW_SLAVE
#ifdef ZW_SLAVE_32
#include <ZW_slave_32_api.h>
#else
#ifdef ZW_SLAVE_ROUTING
#include <ZW_slave_routing_api.h>
#else
#include <ZW_slave_api.h>
#endif  /* ZW_SLAVE_ROUTING */
#endif  /* ZW_SLAVE_32 */
#endif  /* ZW_SLAVE */

#include <ZW_classcmd.h>

#include <ZW_TransportLayer.h>

#include <starter.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/* CHANGE - Change the list of command classes to fit your product */

/* A list of the known command classes. Except the basic class which allways */
/* should be supported. Used when node info is send */
PATCH_VARIABLE_STARTER t_nodeInfo nodeInfo
#if !(defined(WORK_PATCH) || defined(STARTER_PATCH))
 = {COMMAND_CLASS_SWITCH_MULTILEVEL,
    COMMAND_CLASS_SWITCH_ALL,
    COMMAND_CLASS_PROTECTION,
    COMMAND_CLASS_POWERLEVEL}
#endif
;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

PATCH_VARIABLE_STARTER ZW_APPLICATION_TX_BUFFER txBuf;

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

#ifdef LIB_RF_NOTIFY
/*===========================   ApplicationRfNotify   ===========================
**    Notify the application when the radio switch state
**    Called from the Z-Wave PROTOCOL When radio switch from Rx to Tx or from Tx to Rx
**    or when the modulator PA (Power Amplifier) turn on/off
**---------------------------------------------------------------------------------*/
#ifdef STARTER_PATCH
extern void ApplicationRfNotify_Wrapped(
  BYTE rfState          /* IN state of the RF, the available values is as follow:
                               ZW_RF_TX_MODE: The RF switch from the Rx to Tx mode, the modualtor is started and PA is on
                               ZW_RF_PA_ON: The RF in the Tx mode, the modualtor PA is turned on
                               ZW_RF_PA_OFF: the Rf in the Tx mode, the modulator PA is turned off
                               ZW_RF_RX_MODE: The RF switch from Tx to Rx mode, the demodulator is started.*/
)
#ifdef PATCH_ENABLE
reentrant
#endif
;
#endif
void          /*RET Nothing */
PATCH_FUNCTION_NAME_WRAPPER(ApplicationRfNotify)(
  BYTE rfState          /* IN state of the RF, the available values is as follow:
                               ZW_RF_TX_MODE: The RF switch from the Rx to Tx mode, the modualtor is started and PA is on
                               ZW_RF_PA_ON: The RF in the Tx mode, the modualtor PA is turned on
                               ZW_RF_PA_OFF: the Rf in the Tx mode, the modulator PA is turned off
                               ZW_RF_RX_MODE: The RF switch from Tx to Rx mode, the demodulator is started.*/
)
#ifdef PATCH_ENABLE
reentrant
#endif
{
#ifdef PATCH_ENABLE
#pragma asm
PATCH_TABLE_ENTRY_WRAPPER(ApplicationRfNotify)
#pragma endasm
#endif
#ifdef STARTER_PATCH
  ApplicationRfNotify_Wrapped(
    rfState
  );
#endif
}
#endif

/*============================   ApplicationInitHW   ========================
**    Initialization of non Z-Wave module hardware
**
**    Side effects:
**       Returning FALSE from this function will force the API into
**       production test mode.
**--------------------------------------------------------------------------*/
#ifdef STARTER_PATCH
extern BYTE ApplicationInitHW_Wrapped(
  BYTE bWakeupReason       /* IN  Nothing     */
)
#ifdef PATCH_ENABLE
reentrant
#endif
;
#endif
BYTE                       /*RET  TRUE        */
PATCH_FUNCTION_NAME_WRAPPER(ApplicationInitHW)(
  BYTE bWakeupReason       /* IN  Nothing     */
)
#ifdef PATCH_ENABLE
reentrant
#endif
{
#ifdef PATCH_ENABLE
#pragma asm
PATCH_TABLE_ENTRY_WRAPPER(ApplicationInitHW)
#pragma endasm
#endif
#ifdef STARTER_PATCH
  return ApplicationInitHW_Wrapped(bWakeupReason);
#else
  return(TRUE);
#endif
}

/*===========================   ApplicationInitSW   =========================
**    Initialization of the Application Software variables and states
**
**
**
**--------------------------------------------------------------------------*/
#ifdef STARTER_PATCH
extern BYTE ApplicationInitSW_Wrapped(void)
#ifdef PATCH_ENABLE
reentrant
#endif
;
#endif
BYTE                      /*RET  TRUE       */
PATCH_FUNCTION_NAME_WRAPPER(ApplicationInitSW)(void) /* IN   Nothing   */
#ifdef PATCH_ENABLE
reentrant
#endif
{
#ifdef PATCH_ENABLE
#pragma asm
PATCH_TABLE_ENTRY_WRAPPER(ApplicationInitSW)
#pragma endasm
#endif
#ifdef STARTER_PATCH
  return ApplicationInitSW_Wrapped();
#else
  return(TRUE);
#endif
}

/*============================   ApplicationTestPoll   ======================
**    Function description
**      This function is called when the slave enters test mode.
**
**    Side effects:
**       Code will not exit until it is reset
**--------------------------------------------------------------------------*/
#ifdef STARTER_PATCH
extern void ApplicationTestPoll_Wrapped(void)
#ifdef PATCH_ENABLE
reentrant
#endif
;
#endif
void
PATCH_FUNCTION_NAME_WRAPPER(ApplicationTestPoll)(void)
#ifdef PATCH_ENABLE
reentrant
#endif
{
#ifdef PATCH_ENABLE
#pragma asm
PATCH_TABLE_ENTRY_WRAPPER(ApplicationTestPoll)
#pragma endasm
#endif
#ifdef STARTER_PATCH
  ApplicationTestPoll_Wrapped();
#endif
  if (0 == 1) ;  /* Force the compiler to make a LCALL ZW_Patchcheck (not LJMP ZW_Patchcheck) */
}

/*=============================  ApplicationPoll   =========================
**    Application poll function for the slave application
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
#ifdef STARTER_PATCH
extern void ApplicationPoll_Wrapped(void)
#ifdef PATCH_ENABLE
reentrant
#endif
;
#endif
void                    /*RET  Nothing                  */
PATCH_FUNCTION_NAME_WRAPPER(ApplicationPoll)(void) /* IN  Nothing                  */
#ifdef PATCH_ENABLE
reentrant
#endif
{
#ifdef PATCH_ENABLE
#pragma asm
PATCH_TABLE_ENTRY_WRAPPER(ApplicationPoll)
#pragma endasm
#endif
#ifdef STARTER_PATCH
  ApplicationPoll_Wrapped();
#endif
  /* CHANGE - Just a call to the API to make the linker happy, should be
     removed in your product. */
  ZW_Random();
}

/*========================   ApplicationCommandHandler   ====================
**    Handling of a received application commands and requests
**
**
**--------------------------------------------------------------------------*/
#ifdef STARTER_PATCH
extern void Transport_ApplicationCommandHandler_Wrapped(
  BYTE  rxStatus,                 /* IN Frame header info */
#if defined(ZW_CONTROLLER) && !defined(ZW_CONTROLLER_STATIC) && !defined(ZW_CONTROLLER_BRIDGE)
  /* TO#1692 */
  BYTE  destNode,                 /* IN  Frame destination ID, only valid when frame is not Multicast */
#endif
  BYTE  sourceNode,               /* IN Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, the union */
                                  /*    should be used to access the fields */
  BYTE   cmdLength                /* IN Number of command bytes including the command */
)
#ifdef PATCH_ENABLE
reentrant
#endif
;
#endif
void                              /*RET Nothing                  */
PATCH_FUNCTION_NAME_WRAPPER(Transport_ApplicationCommandHandler)(
  BYTE  rxStatus,                 /* IN Frame header info */
#if defined(ZW_CONTROLLER) && !defined(ZW_CONTROLLER_STATIC) && !defined(ZW_CONTROLLER_BRIDGE)
  /* TO#1692 */
  BYTE  destNode,                 /* IN  Frame destination ID, only valid when frame is not Multicast */
#endif
  BYTE  sourceNode,               /* IN Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, the union */
                                  /*    should be used to access the fields */
  BYTE   cmdLength                /* IN Number of command bytes including the command */
)
#ifdef PATCH_ENABLE
reentrant
#endif
{
#ifdef PATCH_ENABLE
#pragma asm
PATCH_TABLE_ENTRY_WRAPPER(Transport_ApplicationCommandHandler)
#pragma endasm
#endif
#ifdef STARTER_PATCH
  Transport_ApplicationCommandHandler_Wrapped(
    rxStatus,
#if defined(ZW_CONTROLLER) && !defined(ZW_CONTROLLER_STATIC) && !defined(ZW_CONTROLLER_BRIDGE)
    destNode,
#endif
    sourceNode,
    pCmd,
    cmdLength
  );
#endif
}


#ifdef ZW_CONTROLLER_BRIDGE
/*======================   ApplicationCommandHandler_Bridge   ================
**    Handling of received application commands and requests
**
**--------------------------------------------------------------------------*/
#ifdef STARTER_PATCH
extern void ApplicationCommandHandler_Bridge_Wrapped(
  BYTE  rxStatus,                 /* IN Frame header info */
  BYTE  destNode,                 /* IN Frame destination ID, only valid when frame is not Multicast */
  BYTE  sourceNode,               /* IN Command sender Node ID */
  ZW_MULTI_DEST *multi,           /* IN multicast structure - only valid if multicast frame */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, */
                                  /*    the command is the very first byte */
  BYTE cmdLength                  /* IN Number of command bytes including the command */
)
#ifdef PATCH_ENABLE
reentrant
#endif
;
#endif
void                              /*RET Nothing                  */
PATCH_FUNCTION_NAME_WRAPPER(ApplicationCommandHandler_Bridge)(
  BYTE  rxStatus,                 /* IN Frame header info */
  BYTE  destNode,                 /* IN Frame destination ID, only valid when frame is not Multicast */
  BYTE  sourceNode,               /* IN Command sender Node ID */
  ZW_MULTI_DEST *multi,           /* IN multicast structure - only valid if multicast frame */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, */
                                  /*    the command is the very first byte */
  BYTE cmdLength                  /* IN Number of command bytes including the command */
)
#ifdef PATCH_ENABLE
reentrant
#endif
{
#ifdef PATCH_ENABLE
#pragma asm
PATCH_TABLE_ENTRY_WRAPPER(ApplicationCommandHandler_Bridge)
#pragma endasm
#endif
#ifdef STARTER_PATCH
  ApplicationCommandHandler_Bridge_Wrapped(
    rxStatus,
    destNode,
    sourceNode,
    multi,
    pCmd,
    cmdLength
  );
#endif
}
#endif


#ifdef ZW_CONTROLLER
/*=====================   ApplicationControllerUpdate   =====================
**    Inform the static controller of node information update done through
**   the network managment.
**
**--------------------------------------------------------------------------*/
#ifdef STARTER_PATCH
extern void ApplicationControllerUpdate_Wrapped(
  BYTE bStatus,                     /* IN   Status of learn mode            */
  BYTE bNodeID,                     /* IN   Node id of node sending nodeinfo*/
  BYTE *pCmd,                       /* IN   Pointer to appl. node info      */
  BYTE bLen                         /* IN   Node info length                */
)
#ifdef PATCH_ENABLE
reentrant
#endif
;
#endif
void                                /* RET  Nothing                         */
PATCH_FUNCTION_NAME_WRAPPER(ApplicationControllerUpdate)(
  BYTE bStatus,                     /* IN   Status of learn mode            */
  BYTE bNodeID,                     /* IN   Node id of node sending nodeinfo*/
  BYTE *pCmd,                       /* IN   Pointer to appl. node info      */
  BYTE bLen                         /* IN   Node info length                */
)
#ifdef PATCH_ENABLE
reentrant
#endif
{
#ifdef PATCH_ENABLE
#pragma asm
PATCH_TABLE_ENTRY_WRAPPER(ApplicationControllerUpdate)
#pragma endasm
#endif
#ifdef STARTER_PATCH
  ApplicationControllerUpdate_Wrapped(
    bStatus,
    bNodeID,
    pCmd,
    bLen
  );
#endif
}
#endif /* ZW_CONTROLLER */


#ifdef ZW_SLAVE
/*==========================   ApplictionSlaveUpdate   =======================
**   Inform a slave application that a node information is received.
**   Called from the slave command handler when a node information frame
**   is received and the Z-Wave protocol is not in a state where it is needed.
**
**--------------------------------------------------------------------------*/
#ifdef STARTER_PATCH
extern void ApplicationSlaveUpdate_Wrapped(
  BYTE bStatus,     /*IN  Status event */
  BYTE bNodeID,     /*IN  Node id of the node that send node info */
  BYTE* pCmd,       /*IN  Pointer to Application Node information */
  BYTE bLen         /*IN  Node info length                        */
)
#ifdef PATCH_ENABLE
reentrant
#endif
;
#endif
void
PATCH_FUNCTION_NAME_WRAPPER(ApplicationSlaveUpdate)(
  BYTE bStatus,     /*IN  Status event */
  BYTE bNodeID,     /*IN  Node id of the node that send node info */
  BYTE* pCmd,       /*IN  Pointer to Application Node information */
  BYTE bLen         /*IN  Node info length                        */
)
#ifdef PATCH_ENABLE
reentrant
#endif
{
#ifdef PATCH_ENABLE
#pragma asm
PATCH_TABLE_ENTRY_WRAPPER(ApplicationSlaveUpdate)
#pragma endasm
#endif
#ifdef STARTER_PATCH
  ApplicationSlaveUpdate_Wrapped(
    bStatus,
    bNodeID,
    pCmd,
    bLen
  );
#endif
}
#endif /* ZW_SLAVE */


/*======================   ApplicationNodeInformation   =====================
**    Request Node information and current status
**    Called by the the Z-Wave application layer before transmitting a
**    "Node Information" frame.
**
**    This is an application function example
**
**--------------------------------------------------------------------------*/
#ifdef STARTER_PATCH
extern void ApplicationNodeInformation_Wrapped(
  BYTE   *deviceOptionsMask,      /*OUT Bitmask with application options     */
  APPL_NODE_TYPE  *nodeType,  /*OUT  Device type Generic and Specific   */
  BYTE       **nodeParm,      /*OUT  Device parameter buffer pointer    */
  BYTE       *parmLength      /*OUT  Number of Device parameter bytes   */
)
#ifdef PATCH_ENABLE
reentrant
#endif
;
#endif
void                  /*RET  Nothing */
PATCH_FUNCTION_NAME_WRAPPER(ApplicationNodeInformation)(
  BYTE   *deviceOptionsMask,      /*OUT Bitmask with application options     */
  APPL_NODE_TYPE  *nodeType,  /*OUT  Device type Generic and Specific   */
  BYTE       **nodeParm,      /*OUT  Device parameter buffer pointer    */
  BYTE       *parmLength      /*OUT  Number of Device parameter bytes   */
)
#ifdef PATCH_ENABLE
reentrant
#endif
{
#ifdef PATCH_ENABLE
#pragma asm
PATCH_TABLE_ENTRY_WRAPPER(ApplicationNodeInformation)
#pragma endasm
#endif
#ifdef STARTER_PATCH
  ApplicationNodeInformation_Wrapped(
    deviceOptionsMask,
    nodeType,
    nodeParm,
    parmLength
  );
#else
  /* CHANGE - Change the parameters in this function to the properties
     of your product. */
     /* this is a listening node And it support optional command classes*/
  *deviceOptionsMask = APPLICATION_NODEINFO_LISTENING | APPLICATION_NODEINFO_OPTIONAL_FUNCTIONALITY;
  nodeType->generic = GENERIC_TYPE_SWITCH_MULTILEVEL;         /* Generic device class */
  nodeType->specific = SPECIFIC_TYPE_POWER_SWITCH_MULTILEVEL; /* Specific device class */
  *nodeParm = (BYTE *)&nodeInfo;        /* Send list of known command classes. */
  *parmLength = sizeof(nodeInfo);       /* Set length*/
#endif
}

#ifdef ZW_CONTROLLER_BRIDGE
/* TODO - Bridge work, need to determine if we want more slave nodetypes */
/* and if this should be handled by sending the nodeinformation request to the */
/* PC, so it is up to the PC to set the nodeinformation when needed */
/* Or it is OK to set nodeinformation from PC side and then start the learning */
/*======================   ApplicationSlaveNodeInformation   =================
**    Request Application Node information and current status
**    Called by the the Z-Wave application layer before transmitting a
**    "Node Information" frame.
**
**--------------------------------------------------------------------------*/
#ifdef STARTER_PATCH
extern void ApplicationSlaveNodeInformation_Wrapped(
  BYTE      destNode,       /* IN Which node do we want the nodeinfo on */
  BYTE   *deviceOptionsMask,     /*OUT Bitmask with application options     */
  APPL_NODE_TYPE *nodeType, /*OUT  Device type Generic and Specific   */
  BYTE      **nodeParm,     /*OUT  Device parameter buffer pointer    */
  BYTE      *parmLength     /*OUT  Number of Device parameter bytes   */
)
#ifdef PATCH_ENABLE
reentrant
#endif
;
#endif
void               /*RET Nothing */
PATCH_FUNCTION_NAME_WRAPPER(ApplicationSlaveNodeInformation)(
  BYTE      destNode,       /* IN Which node do we want the nodeinfo on */
  BYTE   *deviceOptionsMask,     /*OUT Bitmask with application options     */
  APPL_NODE_TYPE *nodeType, /*OUT  Device type Generic and Specific   */
  BYTE      **nodeParm,     /*OUT  Device parameter buffer pointer    */
  BYTE      *parmLength     /*OUT  Number of Device parameter bytes   */
)
#ifdef PATCH_ENABLE
reentrant
#endif
{
#ifdef PATCH_ENABLE
#pragma asm
PATCH_TABLE_ENTRY_WRAPPER(ApplicationSlaveNodeInformation)
#pragma endasm
#endif
#ifdef STARTER_PATCH
  ApplicationSlaveNodeInformation_Wrapped(
    destNode,
    deviceOptionsMask,
    nodeType,
    nodeParm,
    parmLength
  );
#else
//  *deviceOptionsMask = applSlaveNodeInfo_deviceOptionsMask;
//  (*nodeType).generic = applSlaveNodeInfo_nodeType_generic;  /* Generic Device Type */
//  (*nodeType).specific = applSlaveNodeInfo_nodeType_specific;  /* Specific Device Type */
//  *nodeParm = applSlaveNodeInfo_nodeParm;
//  *parmLength = applSlaveNodeInfo_parmLength;
#endif
}
#endif
