/******************************* p_button.h  *******************************
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
 *
 * Description: Button module header file for development kit controller board.
 *
 * Author:   Henrik Holm
 *
 * Last Changed By:  $Author: efh $
 * Revision:         $Revision: 23679 $
 * Last Changed:     $Date: 2012-11-09 13:38:48 +0200 (Пт, 09 ноя 2012) $
 *
 ****************************************************************************/
#ifndef _P_BUTTON_H_
#define _P_BUTTON_H_

#include <ZW_sysdefs.h>
#include <ZW_typedefs.h>

/****************************************************************************/
/*                       PUBLIC TYPES and DEFINITIONS                       */
/****************************************************************************/

/*PUSH BUTTON defines*/
#define DOWN_PB0 0x01
#define DOWN_PB1 0x02
#define DOWN_PB2 0x04
#define DOWN_PB3 0x08
#define DOWN_PB4 0x10
#define PB_RELEASE 0x20
#define PB_HELD  0x40


#define PB_MASK (~(PB_HELD|PB_RELEASE))
#define PB_STATUS_MASK (PB_HELD|PB_RELEASE)

/*Macroes for getting a PB status*/
#define IS_DOWN_PB0(val) (val##&DOWN_PB0)
#define IS_DOWN_PB1(val) (val##&DOWN_PB1)
#define IS_DOWN_PB2(val) (val##&DOWN_PB2)
#define IS_DOWN_PB3(val) (val##&DOWN_PB3)
#define IS_DOWN_PB4(val) (val##&DOWN_PB4)

#define IS_HELD_PB0() (IsButtonHeld(0))
#define IS_HELD_PB1() (IsButtonHeld(1))
#define IS_HELD_PB2() (IsButtonHeld(2))
#define IS_HELD_PB3() (IsButtonHeld(3))
#define IS_HELD_PB4() (IsButtonHeld(4))

#define IS_PB_NOT_HELD(val) (!(val##|~PB_HELD))
#define IS_PB_HELD(val)     (val##&PB_HELD)


#define IS_RELEASED_PB0(val) (!(val##&DOWN_PB0))
#define IS_RELEASED_PB1(val) (!(val##&DOWN_PB1))
#define IS_RELEASED_PB2(val) (!(val##&DOWN_PB2))
#define IS_RELEASED_PB3(val) (!(val##&DOWN_PB3))
#define IS_RELEASED_PB4(val) (!(val##&DOWN_PB4))

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/
/*============================   TimerCheckButtons   ======================
**    Function description
**    This function checks and updates the status of the Push buttons.
**    It is run every 10ms
**    Side effects:
**
**--------------------------------------------------------------------------*/
BYTE initButton(void);

/*============================   ButtonPressed   ============================
**    Function description
**      Checks the status of buttons. If buttons are down they are returned
**      with status of how long they have been pressed down.
**    Side effects:
**
**--------------------------------------------------------------------------*/
BYTE ButtonPressed(void);

/**
 * @brief IsButtonHeld
 * Return status if button is held.
 * @param btnNr description..
 * @return boolean button status
 */
BOOL IsButtonHeld(BYTE btnNr);

#endif /*_P_BUTTON_H_*/
