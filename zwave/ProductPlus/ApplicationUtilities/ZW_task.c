/***************************************************************************
*
* Copyright (c) 2001-2015
* Sigma Designs, Inc.
* All Rights Reserved
*
*---------------------------------------------------------------------------
*
* Description: Task pool handler
*
* Author: Thomas Roll
*
* Last Changed By: $Author: tro $
* Revision: $Revision: 0.00 $
* Last Changed: $Date: 2015/03/02 14:27:01 $
*
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_stdint.h>
#include <ZW_typedefs.h>
#include <ZW_task.h>
#include <ZW_mem_api.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef TASK_DEBUG
#include <ZW_uart_api.h>
#define TD_(x) x
#else
#define TD_(x)
#endif
#define _TD_(x) /*outcommon debug print*/

/**
 * TASK_POOL_JOB includes all informations for a task.
 */
typedef struct _TASK_POOL_JOB_
{
  BYTE id        : 6;
  BYTE active    : 1;
  BYTE notUsed   : 1;
  BOOL (CODE *CBPolltask)(void);
  const char* pTaskName;
} TASK_POOL_JOB;

/**
 * TASK_POOL structure
 */
typedef struct _TASK_POOL_
{
  /**
   * inptSignal is used to signal an interrupt has occur and task pool most
   * be re-checked for jobs.
   * TRUE interrupt has occur
   * FALSE no interrupt has occur
   */
  BYTE intpSignal;
  /**
   * poolJobStatus is used to control pool jobs.
   * TRUE on or more active pool jobs.
   * FALSE no active pool jobs.
   */
  BYTE poolJobStatus;
  BYTE nextSlot;            /** < next task to be run*/
  BYTE usedSlots;
  TASK_POOL_JOB myTaskPool[TASK_POOL_SIZE];
} TASK_POOL;


/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

static TASK_POOL myTask;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

/**
 * @brief TaskAddQueue
 * Add task to task pool.
 * @param CBPolltask function pointer to the task to poll.
 * @param pTaskName pointer to a string describing the Task name.
 * @return task handle ID or 0 if task-pool is full. Increase TASK_POOL_SIZE
 * if pool is full.
 */
uint8_t TaskAdd(BOOL (CODE *CBPolltask)(void), const char * pTaskName)
{
  BYTE i = 0;
  BYTE freeSlot = 0xff;
  BYTE nextId = 0;
  static BOOL isInitialized = FALSE;

  if (TRUE != isInitialized)
  {
    memset((uint8_t *)&myTask, 0x00, sizeof(TASK_POOL));
    isInitialized = TRUE;
  }

  TD_(ZW_DEBUG_SEND_NL());
  TD_(ZW_DEBUG_SEND_STR("TaskAdd>"));
  TD_(ZW_DEBUG_SEND_STR(pTaskName));
  /*Search for a free slot in pool queue*/
  for(i = 0; i < TASK_POOL_SIZE; i++)
  {
    /* find free slot*/
    if((0 == myTask.myTaskPool[i].id) && (0xff == freeSlot))
    {
      freeSlot = i;
    }

    /* find next Id*/
    if( nextId <= myTask.myTaskPool[i].id)
    {
      nextId = myTask.myTaskPool[i].id + 1;
    }

    /*Check if it is in pool*/
    if((CBPolltask == myTask.myTaskPool[i].CBPolltask) &&
        (NULL != CBPolltask))
    {
      /*Task is in pool*/
      myTask.nextSlot = i;
      myTask.poolJobStatus = TRUE; /*Force pool check*/
      /*Break loop and return the task id*/
      freeSlot = 0xff;
      nextId = myTask.myTaskPool[i].id;
	    myTask.myTaskPool[i].pTaskName = pTaskName;
      break;
    }
  }

  /** Add task to pool*/
  if(0xff != freeSlot)
  {
    myTask.myTaskPool[freeSlot].id = nextId;
    myTask.myTaskPool[freeSlot].active = TRUE;
    myTask.myTaskPool[freeSlot].CBPolltask = CBPolltask;
    myTask.myTaskPool[freeSlot].pTaskName = pTaskName;
    myTask.poolJobStatus = TRUE; /*Force pool check*/
    myTask.nextSlot = freeSlot;
    if(freeSlot >= myTask.usedSlots)
    {
      myTask.usedSlots = freeSlot + 1;
    }
  }
  else{
    nextId = 0;
  }
  TD_(ZW_DEBUG_SEND_NUM(nextId));
  TD_(ZW_DEBUG_SEND_NL());
  return nextId;
}


/**
 * @brief TaskPause
 * Pause task.
 * @param taskhandeId task handle ID
 * @return FALSE if taskhandeId is unknown else TRUE.
 */
BOOL TaskPause(BYTE taskhandeId)
{
  BYTE i = 0;
  BOOL status = FALSE;
  TD_(ZW_DEBUG_SEND_NL());
  TD_(ZW_DEBUG_SEND_STR("TaskPause "));
  TD_(ZW_DEBUG_SEND_NUM(taskhandeId));

  for(i = 0; i < myTask.usedSlots; i++)
  {
    /* find Id*/
    if( taskhandeId == myTask.myTaskPool[i].id)
    {
      myTask.myTaskPool[i].active = FALSE;
      status = TRUE;
      break;
    }
  }
  TD_(ZW_DEBUG_SEND_NUM(status));
  TD_(ZW_DEBUG_SEND_NL());
  return status;
}


/**
 * @brief TaskRun
 * Run pause task. If running do nothing.
 * @param taskhandeId task handle ID
 * @return FALSE if taskhandeId is unknown else TRUE.
 */
BOOL TaskRun(BYTE taskhandeId)
{
  BYTE i = 0;
  BOOL status = FALSE;
  TD_(ZW_DEBUG_SEND_NL());
  TD_(ZW_DEBUG_SEND_STR("TaskRun "));
  TD_(ZW_DEBUG_SEND_NUM(taskhandeId));

  for(i = 0; i < myTask.usedSlots; i++)
  {
    /* find Id*/
    if( taskhandeId == myTask.myTaskPool[i].id)
    {
      myTask.myTaskPool[i].active = TRUE;
      status = TRUE;
    }
  }
  TD_(ZW_DEBUG_SEND_NUM(status));
  TD_(ZW_DEBUG_SEND_NL());
  return status;
}


/**
 * @brief TaskRemove
 * Remove task from pool.
 * @param taskhandeId task handle ID
 * @return FALSE if taskhandeId is unknown else TRUE.
 */
BOOL TaskRemove(BYTE taskhandeId)
{
  BYTE i = 0;
  BOOL status = FALSE;
  TD_(ZW_DEBUG_SEND_NL());
  TD_(ZW_DEBUG_SEND_STR("TaskRemove "));
  TD_(ZW_DEBUG_SEND_NUM(taskhandeId));

  for(i = 0; i < myTask.usedSlots; i++)
  {
    /* find Id*/
    if( taskhandeId == myTask.myTaskPool[i].id)
    {
      myTask.myTaskPool[i].id = 0;
      myTask.myTaskPool[i].active = FALSE;
      myTask.myTaskPool[i].CBPolltask = NULL;
	    myTask.myTaskPool[i].pTaskName = NULL;
      myTask.nextSlot = 0;
      status = TRUE;

      /* Count usedSlots down if it is last task in pool.*/
      while((0 == myTask.myTaskPool[i].id) && ((i + 1) ==  myTask.usedSlots))
      {
        myTask.usedSlots--;
        if(0 != i)
        {
          /* check prev slot*/
          --i;
        }
        else
        {
          /*pool empty*/
          break;
        }
      }
    }
  }
  TD_(ZW_DEBUG_SEND_NUM(status));
  TD_(ZW_DEBUG_SEND_NL());
  return status;
}


/**
 * @brief TaskInterruptSignal
 * Interrupt process signal to the Task Handler to run all task in pool
 * to check for new jobs.
 */
void TaskInterruptSignal(void)
{
  myTask.intpSignal = TRUE;
}


/**
 * @brief TaskApplicationPoll
 * Task handler main poll queue.
 * @return FALSE if pool tasks has no jobs or no Interrupt has occur.
 * TRUE if one or more task has job.
 */
BOOL TaskApplicationPoll(void)
{
  BYTE i = 0;

  /* Clear Interrupt flag beacuse we check the pool*/
  myTask.intpSignal = FALSE;

  /* Only initiate poolJobStatus to FALSE if nextId = 0. we
     want to check the whole pool*/
  if(0 == myTask.nextSlot)
  {
    myTask.poolJobStatus = FALSE;
  }
  else{
    myTask.poolJobStatus = TRUE;
  }

  for(i = myTask.nextSlot; i < myTask.usedSlots; i++)
  {
    /* check job*/
    if((0 != myTask.myTaskPool[i].id) &&
       (TRUE == myTask.myTaskPool[i].active) &&
       (NULL != myTask.myTaskPool[i].CBPolltask)
      )
    {
      /*Call task*/
      if(TRUE == myTask.myTaskPool[i].CBPolltask())
      {
        /*Task was active*/
        myTask.poolJobStatus = TRUE;

        if(TASK_POOL_SIZE <= ++myTask.nextSlot)
        {
          myTask.nextSlot = 0;
        }
        TD_(ZW_DEBUG_SEND_BYTE('$'));
//        TD_(ZW_DEBUG_SEND_STR(" id"));
//        TD_(ZW_DEBUG_SEND_NUM(myTask.myTaskPool[i].id));
//        TD_(ZW_DEBUG_SEND_STR(" n"));
//        TD_(ZW_DEBUG_SEND_NUM(myTask.nextSlot));
        return TaskJobFinish();
      }
      else{
        /*No job.. check next*/
      }
    }
  }
  myTask.nextSlot = 0;
//  TD_(ZW_DEBUG_SEND_STR(" n"));
//  TD_(ZW_DEBUG_SEND_NUM(myTask.nextSlot));
  return TaskJobFinish();
}


/**
 * @brief TaskJobFinish
 * Ask task handler if more task job to run. If not go to sleep.
 * @return FALSE if pool tasks has no jobs or no Interrupt has occur.
 * TRUE if one or more task has job.
 */
BOOL TaskJobFinish()
{
  if((FALSE == myTask.intpSignal) && (FALSE == myTask.poolJobStatus))
  {
//    TD_(ZW_DEBUG_SEND_BYTE('+'));
    return TRUE;
  }
//    TD_(ZW_DEBUG_SEND_BYTE('%'));
  return FALSE;
}
