/**
 *
 * Copyright (c) 2001-2014
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 * @file ZW_string.h
 *
 * @brief string utils.
 *
 * Author: Thomas Roll
 *
 * Last Changed By: $Author: tro $
 * Revision: $Revision: 0.00 $
 * Last Changed: $Date: 2014/06/02 10:32:49 $
 *
 */

#ifndef _ZW_STRING_H_
#define _ZW_STRING_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>

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
 * @brief ZW_strlen
 * ZW_strlen computes the length of the string str up to but not including the 
 * terminating null character.
 * @param str This is the string whose length is to be found.
 * @return the length of string.
 */
BYTE ZW_strlen(BYTE* str);


#endif /* _ZW_STRING_H_ */


