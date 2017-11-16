/***************************  ZW_CONTROLLER_STATIC_API.H  *******************
 *           #######
 *           ##  ##
 *           #  ##    ####   #####    #####  ##  ##   #####
 *             ##    ##  ##  ##  ##  ##      ##  ##  ##
 *            ##  #  ######  ##  ##   ####   ##  ##   ####
 *           ##  ##  ##      ##  ##      ##   #####      ##
 *          #######   ####   ##  ##  #####       ##  #####
 *                                           #####
 *          Z-Wave, the wireless lauguage.
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
 *
 * Description: Z-Wave Static Controller node application interface
 *
 * Author:   Johann Sigfredsson
 *
 * Last Changed By:  $Author: jfr $
 * Revision:         $Revision: 34842 $
 * Last Changed:     $Date: 2016-11-22 17:21:37 +0100 (ti, 22 nov 2016) $
 *
 ****************************************************************************/
#ifndef _ZW_CONTROLLER_STATIC_API_H_
#define _ZW_CONTROLLER_STATIC_API_H_

#ifndef ZW_CONTROLLER_STATIC
#define ZW_CONTROLLER_STATIC
#endif

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
/*These are a part of the standard static controller API*/
#include <ZW_controller_api.h>
/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
#define ZW_ROUTE_UPDATE_DONE      0x00
/****************************************************************************
* Functionality specific for the Static Controller API.
****************************************************************************/
/*============================   ZW_RediscoveryNeeded   ======================
**    Function description
**      Application want a rediscovery, but is lost.
**      Try to ask the nodeID supplied for help.
**      Callback function returns:
**          ZW_ROUTE_LOST_FAILED  - if node nodeid is unable to help
**          ZW_ROUTE_LOST_ACCEPT  - if node nodeid contacted the controller.
**          ZW_ROUTE_UPDATE_DONE  - the rediscovery ended successfully.
**          ZW_ROUTE_UPDATE_ABORT - the node nodeid did not answer.
**    Side effects:
**
**--------------------------------------------------------------------------*/
#define ZW_REDISCOVERY_NEEDED(nodeid,func) ZW_RediscoveryNeeded(nodeid,func)

/*============================   ZW_RediscoveryNeeded   ======================
**    Function description
**      Application thinks that a rediscovery is needed.
**      Try to reach the nodeID supplied
**    Side effects:
**
**--------------------------------------------------------------------------*/
BYTE                                                /*RET TRUE if success*/
ZW_RediscoveryNeeded(
  BYTE bNodeID,                                     /*IN nodeID to ask for help*/
  VOID_CALLBACKFUNC(completedFunc)(BYTE txStatus));  /*IN callback function*/


/*========================   ZW_CreateNewPrimaryCtrl   ======================
**
**    Create a new primary controller
**
**    The modes are:
**
**    CREATE_PRIMARY_START          Start the creation of a new primary
**    CREATE_PRIMARY_STOP           Stop the creation of a new primary
**    CREATE_PRIMARY_STOP_FAILED    Report that the replication failed
**
**    ADD_NODE_OPTION_NORMAL_POWER    Set this flag in bMode for High Power inclusion.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
#define ZW_CREATE_NEW_PRIMARY_CTRL(MODE, FUNC) ZW_CreateNewPrimaryCtrl(MODE, FUNC)


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/*                 Implemented within the application moduls                */
/****************************************************************************/

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/*                 Implemented within the Z-Wave controller modules         */
/****************************************************************************/

/*========================   ZW_CreateNewPrimaryCtrl   ======================
**
**    Create a new primary controller
**
**    The modes are:
**
**    CREATE_PRIMARY_START          Start the creation of a new primary
**    CREATE_PRIMARY_STOP           Stop the creation of a new primary
**    CREATE_PRIMARY_STOP_FAILED    Report that the replication failed
**
**    ADD_NODE_OPTION_NORMAL_POWER    Set this flag in bMode for High Power inclusion.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZW_CreateNewPrimaryCtrl(BYTE bMode,
                        VOID_CALLBACKFUNC(completedFunc)(LEARN_INFO*));



#endif /* _ZW_CONTROLLER_STATIC_API_H_ */

