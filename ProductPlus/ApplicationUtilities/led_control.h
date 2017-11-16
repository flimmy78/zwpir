/****************************************************************************
 *
 * Copyright (c) 2001-2013
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: Implements board support functions that controls the LEDs
 *              on the ZDP03A development board.
 *
 * Author:   Erik Friis Harck
 *
 * Last Changed By:  $Author: efh $
 * Revision:         $Revision: 18624 $
 * Last Changed:     $Date: 2010-09-03 12:57:55 +0200 (fr, 03 sep 2010) $
 *
 ****************************************************************************/
#ifndef _LED_CONTROL_H_
#define _LED_CONTROL_H_


/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/


/****************************************************************************/
/*                              EXTERNAL DEFINED FUNCTIONS/DATA             */
/****************************************************************************/

extern BYTE zm4102_mode_enable;

/*===================================   LedOn   =============================
**    This function turns on the specified LED.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
extern void
LedOn(
  BYTE Led                        /* LED to turn on. */
);

/*===================================   LedOff   ============================
**    This function turns off the specified LED.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
extern void
LedOff(
  BYTE Led                        /* LED to turn on. */
);

/*==================================   LedToggle   ==========================
**    This function toggles the specified LED.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
extern void
LedToggle(
  BYTE Led                        /* LED to toggle. */
);

/*===============================   LedControlInit   ========================
**    This function initializes the LED I/O.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
extern void LedControlInit(void);

#endif /*_LED_CONTROL_H_*/
