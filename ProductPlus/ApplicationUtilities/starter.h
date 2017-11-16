/*********************************  starter.h  ******************************
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
 * Description: Include file for slave application
 *
 * Author:   Peter Shorty, Erik Friis Harck
 *
 * Last Changed By:  $Author: psh $
 * Revision:         $Revision: 5038 $
 * Last Changed:     $Date: 2004-04-30 15:34:41 +0200 (Fri, 30 Apr 2004) $
 *
 ****************************************************************************/
#ifndef _STARTER_H_
#define _STARTER_H_

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/* CHANGE - Change the number of classes to fit your product */

/* How many classes do we belong to */
#define CLASS_MEMBERSHIP_COUNT  4

typedef struct s_nodeInfo_
{
   BYTE memberClass[CLASS_MEMBERSHIP_COUNT];  /* Command class membership */
} t_nodeInfo;



/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

#endif /* _STARTER_H */
