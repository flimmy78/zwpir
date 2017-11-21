/**
 * @file
 * Handler for Command Class Association.
 * @copyright Copyright (c) 2001-2016, Sigma Designs Inc., All Rights Reserved
 */

#ifndef _COMMAND_CLASS_ASSOCIATION_H_
#define _COMMAND_CLASS_ASSOCIATION_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_sysdefs.h>
#include <ZW_pindefs.h>
#include <ZW_evaldefs.h>
#include <ZW_classcmd.h>

/**
 * Returns the version of this CC.
 */
#define CommandClassAssociationVersionGet() ASSOCIATION_VERSION_V2

/*==============================   handleCommandClassAssociation  ============
**
**  Function:  handler for Association CC
**
**  Side effects: None
**
**--------------------------------------------------------------------------*/
extern void 
handleCommandClassAssociation(
  BYTE  option,                 /* IN Frame header info */
  BYTE  sourceNode,               /* IN Command sender Node ID */
  ZW_APPLICATION_TX_BUFFER *pCmd, /* IN Payload from the received frame, the union */
  /*    should be used to access the fields */
  BYTE   cmdLength                /* IN Number of command bytes including the command */
);


/*============================ GetLastActiveGroupId ===============================
** Function description
** Report the current active group.
**
** Side effects: 
**
**-------------------------------------------------------------------------*/
extern BYTE 
ApplicationGetLastActiveGroupId(void);


/*============================ handleGetMaxNodesInGroup ====================
** Function description
** Return max nodes supported in a group.
**
** Side effects: 
**
**-------------------------------------------------------------------------*/
extern BYTE
handleGetMaxNodesInGroup(void);


/*============================ CheckgroupIden ===============================
** Function description
** Check if grouidentifier is legal and change *pGroupIden to 1 if 
** MAX_ASSOCIATION_GROUPS = 1.
**
** Side effects: 
**
**-------------------------------------------------------------------------*/
extern BOOL
handleCheckgroupIden(BYTE* pGroupIden);

#endif
