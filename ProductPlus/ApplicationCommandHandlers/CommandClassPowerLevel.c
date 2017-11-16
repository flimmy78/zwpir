/***************************************************************************
 *
 * Copyright (c) 2001-2011
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: Power Level Command Class source file
 *
 * Author:
 *
 * Last Changed By:  $Author:  $
 * Revision:         $Revision:  $
 * Last Changed:     $Date:  $
 *
 ****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_stdint.h>
#include <ZW_basis_api.h>
#include <ZW_tx_mutex.h>
#include <ZW_TransportLayer.h>
#include <CommandClassPowerLevel.h>
#include <ZW_basis_api.h>
#include <ZW_timer_api.h>
#include <misc.h>
#include <ZW_uart_api.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_DEBUG_POW
#define ZW_DEBUG_POW_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_POW_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_POW_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_POW_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_POW_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_POW_SEND_BYTE(data)
#define ZW_DEBUG_POW_SEND_STR(STR)
#define ZW_DEBUG_POW_SEND_NUM(data)
#define ZW_DEBUG_POW_SEND_WORD_NUM(data)
#define ZW_DEBUG_POW_SEND_NL()
#endif

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

static uint8_t timerPowerLevelHandle = 0;
static uint8_t timerPowerLevelSec = 0;
static uint8_t testNodeID = ZW_TEST_NOT_A_NODEID;
static uint8_t DelayTestFrameHandle = 0;
static uint8_t testSourceNodeID;
static uint8_t testPowerLevel;
static uint16_t testFrameCount;
static uint16_t testFrameSuccessCount;
static uint8_t testState = POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_FAILED;
static uint8_t powerLevelBackup = normalPower;
static uint8_t txOption;

VOID_CALLBACKFUNC(pPowStopPowerDownTimer) (void) = NULL;
VOID_CALLBACKFUNC(pPowStartPowerDownTimer) (void) = NULL;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
void ZCB_DelayTestFrame(void);
static void SendTestReport(void);
void ZCB_PowerLevelTimeout(void);
void ZCB_SendTestDone(uint8_t bStatus);
static void composeTestNodeReport(ZW_APPLICATION_TX_BUFFER * pBuffer);

/**
 * Send latest test results.
 */
static void SendTestReport(void)
{
  /*
   * Get a request buffer without handing a callback function because we don't care whether it is
   * received or not. The recipient must ask for another report if this one's lost.
   */
  ZW_APPLICATION_TX_BUFFER *pTxBuf = GetRequestBuffer(NULL);
  /*Check pTxBuf is free*/
  if (IS_NULL(pTxBuf))
  {
    return;
  }

  composeTestNodeReport(pTxBuf);

  if (FALSE == Transport_SendRequest(
                                     testSourceNodeID,
                                     pTxBuf,
                                     sizeof(pTxBuf->ZW_PowerlevelTestNodeReportFrame),
                                     ZWAVE_PLUS_TX_OPTIONS,
                                     ZCB_RequestJobStatus,
                                     FALSE))
  {
    /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
    FreeRequestBuffer();
  }
}

/*=============================   SendTestDone   ============================
 **    Test frame has been transmitted to DUT and the result is noted for
 **    later reporting. If not finished then another Test frame is transmitted.
 **    If all test frames has been transmitted then the test is stopped
 **    and the final result is reported to the PowerlevelTest initiator.
 **
 **    This is an application function example
 **
 **--------------------------------------------------------------------------*/
PCB(ZCB_SendTestDone) (uint8_t bStatus)
{
  if (TRANSMIT_COMPLETE_OK == bStatus)
  {
    testFrameSuccessCount++;
  }
  if (0 != DelayTestFrameHandle)
  {
    TimerCancel(DelayTestFrameHandle);
  }

  if (testFrameCount && (--testFrameCount))
  {
    DelayTestFrameHandle = TimerStart(ZCB_DelayTestFrame, 4, TIMER_ONE_TIME);
  }
  else
  {
    if (testFrameSuccessCount)
    {
      testState = POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_SUCCES;
    }
    else
    {
      testState = POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_FAILED;
    }
    ZW_RFPowerLevelSet(powerLevelBackup); // Restore the original power level.
    SendTestReport();
    if (NON_NULL(pPowStartPowerDownTimer))
    {
      pPowStartPowerDownTimer();
    }
  }
}

/*=======================   PowerLevelTimerCancel   ==========================
 **    Cancels PowerLevel timer
 **
 **    This is an application function example
 **
 **--------------------------------------------------------------------------*/
void
PowerLevelTimerCancel(void)
{
  ZW_DEBUG_POW_SEND_STR("PowerLevelTimerCancel");
  ZW_DEBUG_POW_SEND_NL();
  TimerCancel(timerPowerLevelHandle);
  timerPowerLevelHandle = 0;
  if (NON_NULL(pPowStartPowerDownTimer))
  {
    /*Stop Powerdown timer because Test frame is send*/
    ZW_DEBUG_POW_SEND_STR("pPowStartPowerDownTimer");
    ZW_DEBUG_POW_SEND_NL();
    pPowStartPowerDownTimer();
  }

}

/**
 * @brief Timer callback which maked sure that the RF transmit powerlevel is set back to
 * normalPower after the designated time period.
 */
PCB(ZCB_PowerLevelTimeout) (void)
{
  ZW_DEBUG_POW_SEND_STR("ZCB_PowerLevelTimeout TO ");
  ZW_DEBUG_POW_SEND_NUM(timerPowerLevelSec);
  ZW_DEBUG_POW_SEND_NL();
  if (!--timerPowerLevelSec)
  {
    ZW_RFPowerLevelSet(normalPower); /* Reset powerlevel to normalPower */
    PowerLevelTimerCancel();
  }
}

/**
 * Starts a powerlevel test.
 */
static void StartTest(void)
{
  if (POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_INPROGRESS != testState)
  {
    testState = POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_INPROGRESS;
    powerLevelBackup = ZW_RFPowerLevelGet(); // Backup the current power level.
    ZW_RFPowerLevelSet(testPowerLevel);
    DelayTestFrameHandle = TimerStart(ZCB_DelayTestFrame, 4, TIMER_ONE_TIME);
  }
}

/**
 * Timeout callback with the purpose of delaying the transmission of a test frame to have time for
 * responding to a Powerlevel Test Node Get command.
 */
PCB(ZCB_DelayTestFrame)(void)
{
  DelayTestFrameHandle = 0;
  if (TRUE == ZW_SendTestFrame(testNodeID, testPowerLevel, ZCB_SendTestDone))
  {
    if (NON_NULL(pPowStopPowerDownTimer))
    {
      /*Stop Powerdown timer because Test frame is send*/
      pPowStopPowerDownTimer();
    }
  }
}

/*==============================   loadStatusPowerLevel  ============
 **
 **  Function:  loads power level status from nvram
 **
 **  Side effects: None
 **
 **--------------------------------------------------------------------------*/
void
loadStatusPowerLevel(VOID_CALLBACKFUNC(pStopPowerDownTimer) (void),VOID_CALLBACKFUNC(pStartPowerDownTimer)(void))
{
  ZW_DEBUG_POW_SEND_STR("loadStatusPowerLevel");
  ZW_DEBUG_POW_SEND_NL();
  pPowStopPowerDownTimer = pStopPowerDownTimer;
  pPowStartPowerDownTimer = pStartPowerDownTimer;
  timerPowerLevelSec = 0;
}

/*==============================   loadInitStatusPowerLevel  ============
 **
 **  Function:  loads initial power level status from nvram
 **
 **  Side effects: None
 **
 **--------------------------------------------------------------------------*/
void
loadInitStatusPowerLevel(VOID_CALLBACKFUNC(pStopPowerDownTimer) (void),VOID_CALLBACKFUNC(pStartPowerDownTimer)(void))
{

  ZW_DEBUG_POW_SEND_STR("loadInitStatusPowerLevel");
  ZW_DEBUG_POW_SEND_NL();

  pPowStopPowerDownTimer = pStopPowerDownTimer;
  pPowStartPowerDownTimer = pStartPowerDownTimer;
  timerPowerLevelSec = 0;
  testNodeID = 0;
  testPowerLevel = 0;
  testFrameCount = 0;
  testSourceNodeID = 0;
  testState = POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_FAILED;
}

void handleCommandClassPowerLevel(
                                  uint8_t option,
                                  uint8_t sourceNode,
                                  ZW_APPLICATION_TX_BUFFER * pCmd,
                                  uint8_t cmdLength)
{
  ZW_APPLICATION_TX_BUFFER * pTxBuf;
  txOption = option;

  switch (pCmd->ZW_Common.cmd)
  {
    case POWERLEVEL_SET:

      if (pCmd->ZW_PowerlevelSetFrame.powerLevel <= miniumPower)
      {

        /*Allways cancel timer if receiving POWERLEVEL_SET*/
        if (timerPowerLevelHandle)
        {
          PowerLevelTimerCancel();
          timerPowerLevelSec = 0;
        }

        if (pCmd->ZW_PowerlevelSetFrame.timeout == 0 || /*If timerout is 0 stop test*/
        (pCmd->ZW_PowerlevelSetFrame.powerLevel == normalPower)) /* If powerLevel is normalPower stop test*/
        {
          /* Set in normal mode. Also if we are in normal mode*/
          ZW_RFPowerLevelSet(normalPower);
          timerPowerLevelSec = 0;
        }
        else
        {
          /*Start or Restart test*/
          if (NON_NULL(pPowStopPowerDownTimer))
          {
            /*Stop Powerdown timer because Test frame is send*/
            pPowStopPowerDownTimer();
          }
          timerPowerLevelSec = pCmd->ZW_PowerlevelSetFrame.timeout;
          timerPowerLevelHandle = TimerStart(ZCB_PowerLevelTimeout, TIMER_ONE_SECOND,
          TIMER_FOREVER);
          ZW_RFPowerLevelSet(pCmd->ZW_PowerlevelSetFrame.powerLevel);
        }
      }
    break;

    case POWERLEVEL_GET:
      pTxBuf = GetResponseBuffer();
      /*Check pTxBuf is free*/
      if (NON_NULL(pTxBuf))
      {
        pTxBuf->ZW_PowerlevelReportFrame.cmdClass = COMMAND_CLASS_POWERLEVEL;
        pTxBuf->ZW_PowerlevelReportFrame.cmd = POWERLEVEL_REPORT;
        pTxBuf->ZW_PowerlevelReportFrame.powerLevel = ZW_RFPowerLevelGet();
        pTxBuf->ZW_PowerlevelReportFrame.timeout = timerPowerLevelSec;
        if (FALSE == Transport_SendResponse(
                                            sourceNode,
                                            pTxBuf,
                                            sizeof(pTxBuf->ZW_PowerlevelReportFrame),
                                            option,
                                            ZCB_ResponseJobStatus))
        {
          /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
          FreeResponseBuffer();
        }
      }
    break;

    case POWERLEVEL_TEST_NODE_SET:
      if (POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_INPROGRESS == testState) // 0x02
      {
        return;
      }

      testSourceNodeID = sourceNode;
      testNodeID = pCmd->ZW_PowerlevelTestNodeSetFrame.testNodeid;
      testPowerLevel = pCmd->ZW_PowerlevelTestNodeSetFrame.powerLevel;
      testFrameCount = (((uint16_t)pCmd->ZW_PowerlevelTestNodeSetFrame.testFrameCount1) << 8) & 0xFF00;
      testFrameCount |= ((uint16_t)pCmd->ZW_PowerlevelTestNodeSetFrame.testFrameCount2) & 0x00FF;
      testFrameSuccessCount = 0;

      if (testFrameCount)
      {
        StartTest();
      }
      else
      {
        testState = POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_FAILED;
        SendTestReport();
      }
    break;

    case POWERLEVEL_TEST_NODE_GET:
      pTxBuf = GetResponseBuffer();
      /*Check pTxBuf is free*/
      if (IS_NULL(pTxBuf))
      {
        return;
      }

      composeTestNodeReport(pTxBuf);

      if (FALSE == Transport_SendResponse(
                                          sourceNode,
                                          pTxBuf,
                                          sizeof(pTxBuf->ZW_PowerlevelTestNodeReportFrame),
                                          txOption,
                                          ZCB_ResponseJobStatus))
      {
        /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
        FreeResponseBuffer();
      }
    break;

    default:
      break;
  }
}

/**
 * Composes a Powerlevel Test Node Report into a given buffer.
 * @param pBuffer Pointer to buffer.
 */
static void composeTestNodeReport(ZW_APPLICATION_TX_BUFFER * pBuffer)
{
  pBuffer->ZW_PowerlevelTestNodeReportFrame.cmdClass = COMMAND_CLASS_POWERLEVEL;
  pBuffer->ZW_PowerlevelTestNodeReportFrame.cmd = POWERLEVEL_TEST_NODE_REPORT;
  pBuffer->ZW_PowerlevelTestNodeReportFrame.testNodeid = testNodeID;
  pBuffer->ZW_PowerlevelTestNodeReportFrame.statusOfOperation = testState;
  pBuffer->ZW_PowerlevelTestNodeReportFrame.testFrameCount1 = (uint8_t)(testFrameSuccessCount >> 8);
  pBuffer->ZW_PowerlevelTestNodeReportFrame.testFrameCount2 = (uint8_t)testFrameSuccessCount;
}

/**
 * @brief NOP
 * Comment function...
 * @param par description..
 * @return description..
 */
void NOPV(uint8_t val)
{
  ZW_APPLICATION_TX_BUFFER *pTxBuf = GetRequestBuffer(NULL);
  /*Check pTxBuf is free*/
  if (NON_NULL(pTxBuf))
  {
    pTxBuf->ZW_PowerlevelTestNodeReportFrame.cmdClass = COMMAND_CLASS_NO_OPERATION;
    pTxBuf->ZW_PowerlevelTestNodeReportFrame.cmd = val;
    /* Send Report - we do not care if it gets there or not - if needed report can be requested again */
    if (FALSE == Transport_SendRequest(
                                       testSourceNodeID,
                                       pTxBuf,
                                       2,
                                       ZWAVE_PLUS_TX_OPTIONS,
                                       ZCB_RequestJobStatus,
                                       TRUE))
    {
      /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
      FreeRequestBuffer();
    }
  }
}

void NOP(void)
{
  NOPV(0);
}

BOOL
CommandClassPowerLevelIsInProgress(void)
{
  return ((POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_INPROGRESS == testState) ? TRUE : FALSE);
}

