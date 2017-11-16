/*******************************  ZW_EV_SCHEDULER.H  ************************
 *
 *          Z-Wave, the wireless lauguage.
 *
 *              Copyright (c) 2001-2011
 *              Sigma Designs, Inc.
 *
 *              All Rights Reserved
 *
 *    This source file is subject to the terms and conditions of the
 *    Sigma Designs Software License Agreement which restricts the manner
 *    in which it may be used.
 *
 *---------------------------------------------------------------------------
 *
 * Description: Z-Wave event scheduler module include
 *
 * Last Changed By:  $Author: tro $
 * Revision:         $Revision: 18221 $
 * Last Changed:     $Date: 2010-07-12 14:28:35 +0200 (ma, 12 jul 2010) $
 *
 ****************************************************************************/
#ifndef _ZW_EV_SCHEDULER_H_
#define _ZW_EV_SCHEDULER_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/*================================== EventSchedulerAdd =======================
**  Function to add application call to scheduler
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
BYTE   /* event handle. failing if 0*/
EventSchedulerAdd( VOID_CALLBACKFUNC(pSchedApp)(void));


/*================================== EventSchedulerRemove ====================
**  Function to remove application call from scheduler
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
BOOL
EventSchedulerRemove( BYTE* pHandle);


/*================================== EventScheduler ==========================
**  Scheduler engine.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
EventScheduler(void);

#endif /*_ZW_EV_SCHEDULER_H_*/


