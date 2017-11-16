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
 * Description: Non zero vaers used for the self heal functionality
 *
 * Author:   Samer Seoud
 *
 * Last Changed By:  $Author: jbu $
 * Revision:         $Revision: 15726 $
 * Last Changed:     $Date: 2009-11-24 16:37:12 +0100 (Tue, 24 Nov 2009) $
 *
 ****************************************************************************/
#pragma userclass (xdata = NON_ZERO_VARS_APP)
/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_typedefs.h>

/* Data that must be maintained after powerdown */
XBYTE networkUpdateDownCount ;
XBYTE networkUpdateFailureCount;
XBYTE lostCount;


