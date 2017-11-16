/***************************************************************************
*
* Copyright (c) 2001-2013
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: functionality to check if a cmdClass is present in NodeInfo.
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2013/05/14 15:28:37 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_classcmd.h>
#include <ZW_TransportLayer.h>
#include <ZW_cmd_class_list.h>
/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_CMDCLIST
#include <ZW_uart_api.h>
#define ZW_DEBUG_CMDCLIST_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_CMDCLIST_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_CMDCLIST_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_CMDCLIST_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_CMDCLIST_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_CMDCLIST_SEND_BYTE(data)
#define ZW_DEBUG_CMDCLIST_SEND_STR(STR)
#define ZW_DEBUG_CMDCLIST_SEND_NUM(data)
#define ZW_DEBUG_CMDCLIST_SEND_WORD_NUM(data)
#define ZW_DEBUG_CMDCLIST_SEND_NL()
#endif


/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
//#define TSEC_CMD_HANDLING_SECURE    1

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

BOOL
CheckCmdClass(BYTE cmdClass,
              BYTE *pList,
              BYTE len)
{
  BYTE i;
  for( i = 0; i < len; i++)
  {
    if(*(pList+i) == cmdClass)
    {
      ZW_DEBUG_CMDCLIST_SEND_STR("CheckCmdC success ");
      ZW_DEBUG_CMDCLIST_SEND_NUM(cmdClass);
      ZW_DEBUG_CMDCLIST_SEND_NL();
      return TRUE;
    }
  }
  ZW_DEBUG_CMDCLIST_SEND_STR("CheckCmdClass fail cmd: ");
  ZW_DEBUG_CMDCLIST_SEND_NUM(cmdClass);
  ZW_DEBUG_CMDCLIST_SEND_NUM(len);
  ZW_DEBUG_CMDCLIST_SEND_NL();
  return FALSE;
}

BOOL
CmdClassSupported( BOOL frameSecure, /*secure (TRUE) or nonsecure (FALSE)*/
                BYTE cmdClass, /*incoming frames cmdclass*/
                BYTE* pSecurelist, /*secure list*/
                BYTE securelistLen, /*secure list length*/
                BYTE* pNonSecurelist, /*nonsecure list*/
                BYTE nonSecurelistLen) /*nonsecure list length*/
{
  /*Check if cmd Class are supported in current mode (unsecure or secure)*/
  if( TRUE == frameSecure )
  {
    ZW_DEBUG_CMDCLIST_SEND_STR("\r\nCC CHECK:");
    ZW_DEBUG_CMDCLIST_SEND_NUM(cmdClass);
    ZW_DEBUG_CMDCLIST_SEND_NL();

    /* Check cmdClass is in secure and nosecure lists */
    if(NON_NULL( pSecurelist ))
    {
      if(TRUE == CheckCmdClass(cmdClass, pSecurelist, securelistLen) ||
         COMMAND_CLASS_BASIC == cmdClass)
      {
        return TRUE; /*cmd is supported!*/
      }
    }

    /*Check cmdClass is in nosecure list*/
    if(NON_NULL( pNonSecurelist ))
    {
      return CheckCmdClass(cmdClass, pNonSecurelist, nonSecurelistLen);
    }
  }
  else
  {
    if (NON_SECURE_NODE == GetNodeSecure() && cmdClass == COMMAND_CLASS_BASIC)
    {
      /* Nonsecure node allways support CC Basic. */
      return TRUE;
    }

    /*Check cmdClass is in nosecure list*/
    if(NON_NULL( pNonSecurelist ))
    {
      return CheckCmdClass(cmdClass, pNonSecurelist, nonSecurelistLen);
    }
  }
  ZW_DEBUG_CMDCLIST_SEND_STR("CmdClassSupported fail cmd: ");
  ZW_DEBUG_CMDCLIST_SEND_NUM(cmdClass);
  ZW_DEBUG_CMDCLIST_SEND_NL();

  return FALSE; /* Cmd not in list*/
}
