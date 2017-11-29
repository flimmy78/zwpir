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
#include <ZW_basis_api.h>
#include <ZW_tx_mutex.h>
#include <ZW_TransportLayer.h>
#include "eeprom.h"

#include "config_app.h"
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

static uint8_t testNodeID = ZW_TEST_NOT_A_NODEID;
static uint8_t testPowerLevel;
static uint8_t testSourceNodeID;
static uint16_t testFrameSuccessCount;
static uint8_t testState = POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_FAILED;

static uint8_t timerPowerLevelHandle = 0;
static uint8_t timerPowerLevelSec = 0;
static uint8_t DelayTestFrameHandle = 0
;
static uint16_t testFrameCount;
static uint8_t currentPower = normalPower;

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
void ZCB_SendTestDone(uint8_t bStatus, TX_STATUS_TYPE *txStatusReport);

/*===========================   SendTestReport   ============================
 **    Send current Powerlevel test results
 **
 **    This is an application function example
 **
 **--------------------------------------------------------------------------*/
static void
SendTestReport(void)
{
  ZW_APPLICATION_TX_BUFFER *pTxBuf = GetRequestBuffer(NULL);
  /*Check pTxBuf is free*/
  if (NON_NULL(pTxBuf))
  {
    MULTICHAN_NODE_ID masterNode;
    TRANSMIT_OPTIONS_TYPE_SINGLE_EX txOptionsEx;

    masterNode.node.nodeId = testSourceNodeID;
    masterNode.node.endpoint = 0;
    masterNode.node.BitAddress = 0;
    masterNode.nodeInfo.BitMultiChannelEncap = 0;
    masterNode.nodeInfo.security = GetHighestSecureLevel(ZW_GetSecurityKeys());
    txOptionsEx.txOptions = ZWAVE_PLUS_TX_OPTIONS;
    txOptionsEx.sourceEndpoint = 0;
    txOptionsEx.pDestNode = &masterNode;

    pTxBuf->ZW_PowerlevelTestNodeReportFrame.cmdClass = COMMAND_CLASS_POWERLEVEL;
    pTxBuf->ZW_PowerlevelTestNodeReportFrame.cmd = POWERLEVEL_TEST_NODE_REPORT;
    pTxBuf->ZW_PowerlevelTestNodeReportFrame.testNodeid = testNodeID;
    pTxBuf->ZW_PowerlevelTestNodeReportFrame.statusOfOperation = testState;
    pTxBuf->ZW_PowerlevelTestNodeReportFrame.testFrameCount1 =
        (uint8_t)(testFrameSuccessCount >> 8);
    pTxBuf->ZW_PowerlevelTestNodeReportFrame.testFrameCount2 = (uint8_t)testFrameSuccessCount;

    if (ZW_TX_IN_PROGRESS != Transport_SendRequestEP(
        (uint8_t*)pTxBuf,
        sizeof(pTxBuf->ZW_PowerlevelTestNodeReportFrame),
        &txOptionsEx,
        ZCB_RequestJobStatus))
    {
      /*Free transmit-buffer mutex*/
      FreeRequestBuffer();
    }
  }
  return;
}

/**
 * @brief Test frame has been transmitted to DUT and the result is noted for later reporting. If
 * not finished then another Test frame is transmitted. If all test frames has been transmitted
 * then the test is stopped and the final result is reported to the PowerlevelTest initiator.
 * @param bStatus Status of transmission.
 * @param txStatusReport Status report.
 */
PCB(ZCB_SendTestDone) (
uint8_t bStatus,
                       TX_STATUS_TYPE *txStatusReport
                       )
{
  UNUSED(txStatusReport);

  if (bStatus == TRANSMIT_COMPLETE_OK)
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
    if ((uint8_t)-1 == DelayTestFrameHandle)
    {
    }
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
    ZW_RFPowerLevelSet(currentPower); /* Back to previous setting */
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
  TimerCancel(timerPowerLevelHandle);
  timerPowerLevelHandle = 0;
  if (NON_NULL(pPowStartPowerDownTimer))
  {
    /*Stop Powerdown timer because Test frame is send*/
    pPowStartPowerDownTimer();
  }

}

/**
 * @brief Timer callback which maked sure that the RF transmit powerlevel is set back to
 * normalPower after the designated time period.
 */
PCB(ZCB_PowerLevelTimeout) (void)
{
  if (!--timerPowerLevelSec)
  {
    ZW_RFPowerLevelSet(normalPower); /* Reset powerlevel to normalPower */
    PowerLevelTimerCancel();
  }
}

/*===============================   StartTest   ==============================
 **    Start the powerlevel test run
 **
 **    This is an application function example
 **
 **--------------------------------------------------------------------------*/
static void StartTest(void)
{
  if (POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_INPROGRESS != testState)
  {
    testState = POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_INPROGRESS;
    currentPower = ZW_RFPowerLevelGet(); /* Get current (normalPower) */
    ZW_RFPowerLevelSet(testPowerLevel);
    DelayTestFrameHandle = TimerStart(ZCB_DelayTestFrame, 4, TIMER_ONE_TIME);
  }
}

/**
 * @brief delay test frame 10 mSec for open up a window responding Get commands on the node.
 */
PCB(ZCB_DelayTestFrame) (void)
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

void loadStatusPowerLevel(VOID_CALLBACKFUNC(pStopPowerDownTimer) (void),VOID_CALLBACKFUNC(pStartPowerDownTimer)(void))
{
  pPowStopPowerDownTimer = pStopPowerDownTimer;
  pPowStartPowerDownTimer = pStartPowerDownTimer;
  timerPowerLevelSec = 0;
}

void loadInitStatusPowerLevel(VOID_CALLBACKFUNC(pStopPowerDownTimer)(void), VOID_CALLBACKFUNC(pStartPowerDownTimer)(void))
{
  pPowStopPowerDownTimer = pStopPowerDownTimer;
  pPowStartPowerDownTimer = pStartPowerDownTimer;
  timerPowerLevelSec = 0;
  testNodeID = 0;
  testPowerLevel = 0;
  testFrameCount = 0;
  testSourceNodeID = 0;
  testState = POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_FAILED;
}

received_frame_status_t
handleCommandClassPowerLevel(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength)
{
  UNUSED(cmdLength);

  if (TRUE == Check_not_legal_response_job(rxOpt))
  {
    /*Do not support endpoint bit-addressing */
    return RECEIVED_FRAME_STATUS_FAIL;
  }

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
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
      return RECEIVED_FRAME_STATUS_FAIL;
    break;

    case POWERLEVEL_GET:
      {
      ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
      /*Check pTxBuf is free*/
      if (NON_NULL(pTxBuf))
      {
        TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
        RxToTxOptions(rxOpt, &pTxOptionsEx);
        pTxBuf->ZW_PowerlevelReportFrame.cmdClass = COMMAND_CLASS_POWERLEVEL;
        pTxBuf->ZW_PowerlevelReportFrame.cmd = POWERLEVEL_REPORT;
        pTxBuf->ZW_PowerlevelReportFrame.powerLevel = ZW_RFPowerLevelGet();
        pTxBuf->ZW_PowerlevelReportFrame.timeout = timerPowerLevelSec;
        if (ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
                                                          (uint8_t *)pTxBuf,
                                                          sizeof(pTxBuf->ZW_PowerlevelReportFrame),
                                                          pTxOptionsEx,
                                                          ZCB_ResponseJobStatus))
        {
          /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
          FreeResponseBuffer();
        }
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }
    }
    return RECEIVED_FRAME_STATUS_FAIL;
    break;

    case POWERLEVEL_TEST_NODE_SET:
      if (POWERLEVEL_TEST_NODE_REPORT_ZW_TEST_INPROGRESS == testState) // 0x02
      {
        return RECEIVED_FRAME_STATUS_SUCCESS;
      }

      testSourceNodeID = rxOpt->sourceNode.nodeId;
      testNodeID = pCmd->ZW_PowerlevelTestNodeSetFrame.testNodeid;
      testPowerLevel = pCmd->ZW_PowerlevelTestNodeSetFrame.powerLevel;
      testFrameCount = (((uint16_t)pCmd->ZW_PowerlevelTestNodeSetFrame.testFrameCount1) << 8);
      testFrameCount |= (uint16_t)pCmd->ZW_PowerlevelTestNodeSetFrame.testFrameCount2;
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
      return RECEIVED_FRAME_STATUS_SUCCESS;
    break;

    case POWERLEVEL_TEST_NODE_GET:
      {
        ZW_APPLICATION_TX_BUFFER *pTxBuf = GetResponseBuffer();
        TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx;
        /*Check pTxBuf is free*/
        if (IS_NULL(pTxBuf))
        {
          return RECEIVED_FRAME_STATUS_FAIL;
        }

        RxToTxOptions(rxOpt, &pTxOptionsEx);
        pTxBuf->ZW_PowerlevelTestNodeReportFrame.cmdClass = COMMAND_CLASS_POWERLEVEL;
        pTxBuf->ZW_PowerlevelTestNodeReportFrame.cmd = POWERLEVEL_TEST_NODE_REPORT;
        pTxBuf->ZW_PowerlevelTestNodeReportFrame.testNodeid = testNodeID;
        pTxBuf->ZW_PowerlevelTestNodeReportFrame.statusOfOperation = testState;
        pTxBuf->ZW_PowerlevelTestNodeReportFrame.testFrameCount1 = (uint8_t)(testFrameSuccessCount
                                                                             >> 8);
        pTxBuf->ZW_PowerlevelTestNodeReportFrame.testFrameCount2 = (uint8_t)testFrameSuccessCount;

        if (ZW_TX_IN_PROGRESS != Transport_SendResponseEP(
            (uint8_t *)pTxBuf,
            sizeof(pTxBuf->ZW_PowerlevelTestNodeReportFrame),
            pTxOptionsEx,
            ZCB_ResponseJobStatus))
        {
          /*Job failed, free transmit-buffer pTxBuf by cleaing mutex */
          FreeResponseBuffer();
        }
      }
      return RECEIVED_FRAME_STATUS_SUCCESS;
    break;

    default:
      break;
  }
  return RECEIVED_FRAME_STATUS_NO_SUPPORT;
}

void NOPV(uint8_t val)
{
  ZW_APPLICATION_TX_BUFFER *pTxBuf = GetRequestBuffer(NULL);
  /*Check pTxBuf is free*/
  if (NON_NULL(pTxBuf))
  {
    TRANSMIT_OPTIONS_TYPE sTxOptions;
    /**
     *  Build transmit options
     */
    sTxOptions.destNode = testSourceNodeID;
    sTxOptions.txOptions = ZWAVE_PLUS_TX_OPTIONS;
    sTxOptions.txSecOptions = 0;
    sTxOptions.securityKey = SECURITY_KEY_NONE;

    pTxBuf->ZW_PowerlevelTestNodeReportFrame.cmdClass = COMMAND_CLASS_NO_OPERATION;
    pTxBuf->ZW_PowerlevelTestNodeReportFrame.cmd = val;

    if (ZW_TX_IN_PROGRESS != ZW_SendDataEx((uint8_t *)pTxBuf,
                                           2,
                                           &sTxOptions,
                                           NULL))
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
