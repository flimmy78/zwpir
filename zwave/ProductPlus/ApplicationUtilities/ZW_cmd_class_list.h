/**
 *
 * Copyright (c) 2001-2014
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 * @file ZW_cmd_class_list.h
 *
 * @brief Validation of command class agains application NIF's
 *
 * Author: Thomas Roll
 *
 * Last Changed By: $Author: tro $
 * Revision: $Revision: 0.00 $
 * Last Changed: $Date: 2014/12/08 12:54:03 $
 *
 */


#ifndef _ZW_CMD_CLASS_LIST_H_
#define _ZW_CMD_CLASS_LIST_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/


/**
 * @brief CheckCmdClass
 * Check if a cmdClass is present in Node-Info cmdClass-list.
 * @param[in] cmdClass cmd to be check
 * @param[in] pList pointer to cmdClass-list (nosecure or secure list)
 * @param[in] len length of the list
 * @return TRUE if prensent else FALSE
 */
BOOL CheckCmdClass(BYTE cmdClass,
                   BYTE *pList,
                   BYTE len);

/**
 * @brief CmdClassSupported
 * Check incoming frame command class is in secure- or nonsecure-list
 * @param[in] frameSecure, secure (TRUE) or nonsecure (FALSE)
 * @param[in] cmdClass incoming frames cmdclass
 * @param[in] pSecurelist secure list
 * @param[in] securelistLen secure list length
 * @param[in] pNonSecurelist nonsecure list
 * @param[in] nonSecurelistLen nonsecure list length
 * @return boolean if commad class is in list.
 */
BOOL
CmdClassSupported(BOOL frameSecure,
                  BYTE cmdClass,
                  BYTE* pSecurelist,
                  BYTE securelistLen,
                  BYTE* pNonSecurelist,
                  BYTE nonSecurelistLen);

#endif /* _ZW_CMD_CLASS_LIST_H_ */
