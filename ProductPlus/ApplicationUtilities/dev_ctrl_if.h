/*******************************  DEV_CTRL_IF.H  *******************************
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
 * Description: IO definitions for the Z-Wave Generic development board
 *
 * Author:   Henrik Holm
 *
 * Last Changed By:  $Author: sse $
 * Revision:         $Revision: 19715 $
 * Last Changed:     $Date: 2010-12-14 17:01:57 +0200 (Вт, 14 дек 2010) $
 *
 ****************************************************************************/
#ifndef _DEV_CTRL_IF_H_
#define _DEV_CTRL_IF_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_pindefs.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
/* Push button pins */
#define PB0Port  P2
//#define PB0shadow P2Shadow
#define PB0SHADOW P2Shadow
#define PB0SHADOWDIR  P2ShadowDIR
#define PB0DIR   P2DIR
#define PB0DIR_PAGE   P2DIR_PAGE
#define PB0      4
#ifdef HW_RF
#define PB1Port  P2
//#define PB1shadow P2Shadow
#define PB1SHADOW P2Shadow
#define PB1SHADOWDIR  P2ShadowDIR
#define PB1DIR   P2DIR
#define PB1DIR_PAGE   P2DIR_PAGE
#define PB1      3

#define PB2Port  P2
//#define PB2shadow P2Shadow
#define PB2SHADOW P2Shadow
#define PB2SHADOWDIR  P2ShadowDIR
#define PB2DIR   P2DIR
#define PB2DIR_PAGE   P2DIR_PAGE
#define PB2      2

#define PB3Port  P2
//#define PB3shadow P2Shadow
#define PB3SHADOW P2Shadow
#define PB3SHADOWDIR  P2ShadowDIR
#define PB3DIR   P2DIR
#define PB3DIR_PAGE   P2DIR_PAGE
#define PB3      1

#define PB4Port  P3
//#define PB4shadow P3Shadow
#define PB4SHADOW P3Shadow
#define PB4SHADOWDIR  P3ShadowDIR
#define PB4DIR   P3DIR
#define PB4DIR_PAGE   P3DIR_PAGE
#define PB4      4
#else
#define PB1Port  P3
//#define PB1shadow P3Shadow
#define PB1SHADOW P3Shadow
#define PB1SHADOWDIR  P3ShadowDIR
#define PB1DIR   P3DIR
#define PB1DIR_PAGE   P3DIR_PAGE
#define PB1      6

#define PB2Port  P2
//#define PB2shadow P2Shadow
#define PB2SHADOW P2Shadow
#define PB2SHADOWDIR  P2ShadowDIR
#define PB2DIR   P2DIR
#define PB2DIR_PAGE   P2DIR_PAGE
#define PB2      3

#define PB3Port  P2
//#define PB3shadow P2Shadow
#define PB3SHADOW P2Shadow
#define PB3SHADOWDIR  P2ShadowDIR
#define PB3DIR   P2DIR
#define PB3DIR_PAGE   P2DIR_PAGE
#define PB3      2

#define PB4Port  P2
//#define PB4shadow P2Shadow
#define PB4SHADOW P2Shadow
#define PB4SHADOWDIR  P2ShadowDIR
#define PB4DIR   P2DIR
#define PB4DIR_PAGE   P2DIR_PAGE
#define PB4      1
#endif
#ifdef HW_RF
#define LED0Port P1
//#define LED0shadow P1Shadow
#define LED0SHADOW P1Shadow
#define LED0SHADOWDIR  P1ShadowDIR
#define LED0DIR  P1DIR
#define LED0DIR_PAGE   P1DIR_PAGE
#define LED0     0

#define LED1Port P1
//#define LED1shadow P1Shadow
#define LED1SHADOW P1Shadow
#define LED1SHADOWDIR  P1ShadowDIR
#define LED1DIR  P1DIR
#define LED1DIR_PAGE   P1DIR_PAGE
#define LED1     2
#else
#define LED0Port P0
//#define LED0shadow P0Shadow
#define LED0SHADOW P0Shadow
#define LED0SHADOWDIR  P0ShadowDIR
#define LED0DIR  P0DIR
#define LED0DIR_PAGE   P0DIR_PAGE
#define LED0     7

#define LED1Port P3
//#define LED1shadow P3Shadow
#define LED1SHADOW P3Shadow
#define LED1SHADOWDIR  P3ShadowDIR
#define LED1DIR  P3DIR
#define LED1DIR_PAGE   P3DIR_PAGE
#define LED1     7
#endif

#define LED2Port P1
#define LED2SHADOW P1Shadow
#define LED2SHADOWDIR  P1ShadowDIR
#define LED2DIR  P1DIR
#define LED2DIR_PAGE   P1DIR_PAGE
#define LED2     0

#define EEPCSPort P2
//#define EEPCSshadow P2Shadow
#define EEPCSSHADOW P2Shadow
#define EEPCSSHADOWDIR  P2ShadowDIR
#define EEPCSDIR  P2DIR
#define EEPCSDIR_PAGE   P2DIR_PAGE
#define EEPCS     5

#endif /* __H_ */
