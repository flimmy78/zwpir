/*******************************  ZW_PORTPIN_API.H  *************************
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
 * Description: ZW040x Port Pin service functions module include
 *
 * Last Changed By:  $Author: jsi $
 * Revision:         $Revision: 18221 $
 * Last Changed:     $Date: 2010-07-12 14:28:35 +0200 (ma, 12 jul 2010) $
 *
 ****************************************************************************/
#ifndef _ZW_PORT_MONITOR_H_
#define _ZW_PORT_MONITOR_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_portpin_api.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/


#define PORT_GRP 4


typedef enum
{
  ID_EVENT_PIO_PORT = 1
} ID_EVENT_MODULE;


typedef enum
{
  ID_PORTIN_CHANGE = 1, /*data struct PORT_STATUS*/
  ID_PORTOUT_CHANGE = 2 /*data struct PORT_STATUS*/
} PORT_EVENT;


typedef struct _PORT_STATUS
{
  BYTE status[PORT_GRP];
  BYTE mask[PORT_GRP];
} tPORT_STATUS;

/*==============================   ZW_PortInit     ===========================
** Function used to setup a function pointer to handle pin in events. Callback
** is per default disabled or can be disable by calling ZW_PortInit(NULL)
**    Side effects: None
**
**--------------------------------------------------------------------------*/
void
ZW_PortMonitorInit( VOID_CALLBACKFUNC(pEventHandler)(WORD, XBYTE*, BYTE));


/*===============================   ZW_PortMonitorPinIn  =====================
**    Setup bPortPin portpin as Input
**
**--------------------------------------------------------------------------*/
void
ZW_PortMonitorPinIn(ENUM_PORTPINS bPortPin);


/*===============================   ZW_PortMonitorPinOut   ===================
**    Setup bPortPin portpin as Output
**
**--------------------------------------------------------------------------*/
void
ZW_PortMonitorPinOut(ENUM_PORTPINS bPortPin);

#endif /*_ZW_PORT_MONITOR_H_*/
