/*************************************************************************** 
* 
* Copyright (c) 2013
* Sigma Designs, Inc. 
* All Rights Reserved 
* 
*--------------------------------------------------------------------------- 
* 
* Description: Some nice descriptive description. 
* 
* Author:   Jakob Buron 
* 
* Last Changed By:  $Author: jdo $ 
* Revision:         $Revision: 1.38 $ 
* Last Changed:     $Date: 2005/07/27 15:12:54 $ 
* 
****************************************************************************/
#ifndef ZW_SECURITY_API_H_
#define ZW_SECURITY_API_H_

enum SECURITY_SCHEME {
  NO_SCHEME = 0x00,
  SECURITY_SCHEME_0 = 0x01
};

#define MASK_SCHEME_0 0x01

#define SECURITY_OPTION_ALLOW_NONSECURE 0x01
#define SECURITY_OPTION_FORCE_NONSECURE 0x02

#define ANY_SECURITY_SCHEME 0xFF

/*===================   ApplicationSecureCommandsSupported   =================
**
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
BYTE                      /* RET TRUE if endpoint is valid, FALSE otherwise */
ApplicationSecureCommandsSupported(
    BYTE endpoint,        /* IN endpoint to report on Range: 1..N */
    BYTE **pCmdClasses,   /* OUT Cmd classes supported by endpoint */
    BYTE *pLength);       /* OUT Length of pCmdClasses, 0 if endpoint does not exist */

/*========================   ZW_GetSecuritySchemes   =====================
**
**    Returns a bitmask of security schemes the application can
**    request for ZW_SendData() calls. When the slave is excluded,
**    no schemes will be reported.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
BYTE ZW_GetSecuritySchemes();

#endif /* ZW_SECURITY_API_H_ */
