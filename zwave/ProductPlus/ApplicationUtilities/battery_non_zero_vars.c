/******************************* self_heal.c *******************************
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
 *              Copyright (c) 2006
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
 * Description: Non zero vaers used for the battery functionality
 *
 * Author:   Samer Seoud
 *
 * Last Changed By:  $Author: jbu $
 * Revision:         $Revision: 15726 $
 * Last Changed:     $Date: 2009-11-24 16:37:12 +0100 (Tue, 24 Nov 2009) $
 *
 ****************************************************************************/
#ifdef __C51__
#pragma userclass (xdata = NON_ZERO_VARS_APP)
#endif
/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_typedefs.h>

/* Data that must be maintained after powerdown */
/* WUT count, when decreased to 0, wake up */
//TO3500
XDWORD wakeupCount;

//counter used as the time window used for shap the event frames traffic
XBYTE bTimeWindow;

/*This flag is set to TRUE when we have received ack from CSC about Battery report frame
  The flag will only be reset after power recycle*/
XBYTE lowBattReportAcked;

/* Battery monitor state */
XBYTE st_battery;

