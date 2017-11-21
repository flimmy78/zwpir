
/***************************************************************************
*
* Copyright (c) 2001-2013
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Protected transmitbuffer used to send data.Use this module to get 
* a transmit buffer and release the buffer again when data is send and application
* is notified with a callback. There are 2 types for buffers. one for unsoliceted  
* events (GetAppCmdFramePointer) and one response-buffer (GetResponseCmdFramePointer)
* for responding a Get command with a Report.
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2013/06/06 10:45:02 $
*
****************************************************************************/
#ifndef _ZW_MUTEX_H_
#define _ZW_MUTEX_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_classcmd.h>


/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
typedef enum _JOB_STATUS_ 
{
  JOB_STATUS_BUSY = 0,
  JOB_STATUS_SUCCESS
} JOB_STATUS;
  
  

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/** 
 * @brief GetAppCmdFramePointer
 * Get pointer to Application tranmit buffer. If return NULL is a job busy and 
 * current action should be cancel.
 * @param completedFunc function-pointer to to return status on job.
 * @return pointer to tranmit-buffer. If NULL is it not free.
 */
ZW_APPLICATION_TX_BUFFER*
GetRequestBuffer( VOID_CALLBACKFUNC(completedFunc)(BYTE) );


/** 
 * @brief FreeApplTransmitBuffer
 * Free transmit buffer by clear mutex and remove callback. This should be
 * called if ZW_SendData() return FALSE. 
 */
void
FreeRequestBuffer(void);


/** 
 * @brief ZCB_AppJobStatus
 * This function must be used a Call-back status function for GetAppCmdFramePointer
 * when calling Z-Wave API ZW_SendData().
 * @param txStatus status on ZW_SendData job.
 */
void
ZCB_RequestJobStatus(BYTE txStatus);

/** 
 * @brief GetCmdTxFramePointer
 * Get transmit buffer for response job. Return NULL if buffer is busy.
 * @return pointer to ZW_APPLICATION_TX_BUFFER.
 */
ZW_APPLICATION_TX_BUFFER*
GetResponseBuffer(void);


/** 
 * @brief GetResponseCmdFramePointerCb
 * Get transmit buffer for response job. Return NULL if buffer is busy.
 * @param completedFunc function-pointer to to return status on job.
 * @return pointer to ZW_APPLICATION_TX_BUFFER.
 */
ZW_APPLICATION_TX_BUFFER*
GetResponseBufferCb(VOID_CALLBACKFUNC(completedFunc)(BYTE));

/** 
 * @brief ZCB_JobResponseStatus
 * Handdle all response jobs. Response-function should be use as function 
 * callback on SendData() call.
 * @param txStatus status on SendData.
 */
void
ZCB_ResponseJobStatus(BYTE txStatus);

/** 
 * @brief FreeResponseTransmitBuffer
 * Cancel job by clear mutex and remove callback.
 */
void
FreeResponseBuffer(void);

/** 
 * @brief ActiveJobs
 * Af is mutex has active jobs by chekking if transmitbuffers are occupied.
 * @return TRUE if on orm ore jobs are active else FALSE.
 */
BOOL ActiveJobs(void);

#endif /* _ZW_MUTEX_H_ */

