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
* Last Changed: $Date: 2013/06/06 10:40:19 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_tx_mutex.h>
#include <ZW_uart_api.h>
#include <ZW_mem_api.h>
#include <misc.h>

#ifdef ZW_DEBUG_MUTEX
#define ZW_DEBUG_MUTEX_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_MUTEX_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_MUTEX_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_MUTEX_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_MUTEX_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_MUTEX_SEND_BYTE(data)
#define ZW_DEBUG_MUTEX_SEND_STR(STR)
#define ZW_DEBUG_MUTEX_SEND_NUM(data)
#define ZW_DEBUG_MUTEX_SEND_WORD_NUM(data)
#define ZW_DEBUG_MUTEX_SEND_NL()
#endif

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
typedef struct _MUTEX
{
  BYTE mutexAppActive;
  BYTE mutexResponseActive;
  VOID_CALLBACKFUNC(pAppJob)(BYTE);
  VOID_CALLBACKFUNC(pResponseJob)(BYTE);
  ZW_APPLICATION_TX_BUFFER appTxBuf;
  ZW_APPLICATION_TX_BUFFER responseTxBuf;
} MUTEX;

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
MUTEX myMutex = {FALSE, FALSE, NULL, NULL};
/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
BOOL MutexSet(BYTE* pMutex );
BOOL MutexActive(BYTE mutex );
void MutexFree(BYTE* pMutex);



/**
 * @brief GetAppCmdFramePointer
 */
ZW_APPLICATION_TX_BUFFER*
GetRequestBuffer( VOID_CALLBACKFUNC(completedFunc)(BYTE) )
{
   ZW_DEBUG_MUTEX_SEND_STR("GetRequestBuffer");
   ZW_DEBUG_MUTEX_SEND_NL();
  /*Set mutex*/
  if(FALSE == MutexSet(&myMutex.mutexAppActive))
  {
    /*Mutex is not free.. stop current job*/
    ZW_DEBUG_MUTEX_SEND_STR("Mutex App not free!");
    ZW_DEBUG_MUTEX_SEND_NL();
    return NULL;
  }
  myMutex.pAppJob = completedFunc;
  ZW_DEBUG_MUTEX_SEND_STR("aMutexOn");
  ZW_DEBUG_MUTEX_SEND_NL();
  memset(&myMutex.appTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER));

  return &myMutex.appTxBuf;
}

/**
 * @brief ZCB_RequestJobStatus
 */
PCB(ZCB_RequestJobStatus)(BYTE txStatus)
{
  ZW_DEBUG_MUTEX_SEND_STR("ZCB_RequestJobStatus");
  ZW_DEBUG_MUTEX_SEND_NL();
  if( NULL != myMutex.pAppJob)
  {
    myMutex.pAppJob(txStatus);
  }
  /*Free Mutex*/
  FreeRequestBuffer();
}


/**
 * @brief FreeApplTransmitBuffer
 * Cancel job by clear mutex and remove callback.
 */
void
FreeRequestBuffer(void)
{
  ZW_DEBUG_MUTEX_SEND_STR("FreeRequestBuffer");
  ZW_DEBUG_MUTEX_SEND_NL();
  /*Remove application func-callback. User should not be called any more*/
  myMutex.pAppJob = NULL;
  /*Free Mutex*/
  MutexFree(&myMutex.mutexAppActive);
}




/**
 * @brief GetResponseBuffer
 */
ZW_APPLICATION_TX_BUFFER*
GetResponseBuffer(void)
{
  return GetResponseBufferCb(NULL);
}


/**
 * GetResponseBufferCb
 */
ZW_APPLICATION_TX_BUFFER*
GetResponseBufferCb(VOID_CALLBACKFUNC(completedFunc)(BYTE))
{
   ZW_DEBUG_MUTEX_SEND_STR("GetResponseBufferCb ");
   ZW_DEBUG_MUTEX_SEND_NUM((BYTE)completedFunc);
   ZW_DEBUG_MUTEX_SEND_NL();
  /*Set mutex*/
#ifdef SECURITY
  if(FALSE == MutexSet(&myMutex.mutexAppActive))
#else
  if(FALSE == MutexSet(&myMutex.mutexResponseActive))
#endif
  {
    /*Mutex is not free.. stop current job*/
    ZW_DEBUG_MUTEX_SEND_STR("Mutex RES not free!");
    ZW_DEBUG_MUTEX_SEND_NL();
    return NULL;
  }
  myMutex.pResponseJob = completedFunc;
  ZW_DEBUG_MUTEX_SEND_STR("rMutexOn");
  ZW_DEBUG_MUTEX_SEND_NL();
  memset(&myMutex.responseTxBuf, 0, sizeof(ZW_APPLICATION_TX_BUFFER) );
  return &myMutex.responseTxBuf;
}

/**
 * ZCB_ResponseJobStatus
 */
PCB(ZCB_ResponseJobStatus)(BYTE txStatus)
{
  VOID_CALLBACKFUNC(tmpfunc)(BYTE);
  ZW_DEBUG_MUTEX_SEND_STR("rMCB ");
  ZW_DEBUG_MUTEX_SEND_NL();
  /*Free Mutex*/
  tmpfunc = myMutex.pResponseJob;
  FreeResponseBuffer();
  if(NON_NULL(tmpfunc))
  {
    tmpfunc(txStatus);
  }
}

/**
 * @brief FreeResponseBuffer
 * Cancel job by clear mutex and remove callback.
 */
void
FreeResponseBuffer(void)
{
  ZW_DEBUG_MUTEX_SEND_STR("FreeResponseBuffer");
  ZW_DEBUG_MUTEX_SEND_NL();
  /*Remove application func-callback. User should not be called any more*/
  myMutex.pResponseJob = NULL;
  /*Free Mutex*/
#ifdef SECURITY
  MutexFree(&myMutex.mutexAppActive);
#else
  MutexFree(&myMutex.mutexResponseActive);
#endif
}


/**
 * @brief MutexSet
 * Set mutex if it is not active
 * @param pMutex pointer to the mutex-flag that should be changed.
 * @return TRUE if mutex was set else FALSE for mutex was not free.
 */
BOOL
MutexSet(BYTE* pMutex)
{
  if( FALSE == *pMutex)
  {
    *pMutex = TRUE;
    return TRUE;
  }
  return FALSE;
}

/**
 * @brief MutexFree
 * Ask state on mutex
 * @param pMutex pointer to the mutex-flag that should be changed.
 * @return mutex state.
 */
BOOL
MutexActive(BYTE mutex )
{
  return mutex;
}

/**
 * @brief MutexFree
 * @param pMutex pointer to the mutex-flag that should be changed.
 * Free mutex
 */
void
MutexFree(BYTE* pMutex)
{
  ZW_DEBUG_MUTEX_SEND_STR("MutexFree ");
  ZW_DEBUG_MUTEX_SEND_NUM((BYTE)pMutex);
  ZW_DEBUG_MUTEX_SEND_NL();
  *pMutex = FALSE;
}



/**
 * @brief ActiveJobs
 * Af is mutex has active jobs by chekking if transmitbuffers are occupied.
 * @return TRUE if on orm ore jobs are active else FALSE.
 */
BOOL ActiveJobs(void)
{
  if((TRUE == myMutex.mutexAppActive) || (TRUE == myMutex.mutexResponseActive))
  {
    return TRUE;
  }
  return FALSE;

}

