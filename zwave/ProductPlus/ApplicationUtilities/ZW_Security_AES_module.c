/***********************  ZW_Security_AES_module.c  *************************
 *           #######
 *           ##  ##
 *           #  ##    ####   #####    #####  ##  ##   #####
 *             ##    ##  ##  ##  ##  ##      ##  ##  ##
 *            ##  #  ######  ##  ##   ####   ##  ##   ####
 *           ##  ##  ##      ##  ##      ##   #####      ##
 *          #######   ####   ##  ##  #####       ##  #####
 *                                           #####
 *          Z-Wave, the wireless language.
 *
 *              Copyright (c) 2001
 *              Zensys A/S
 *              Denmark
 *
 *              All Rights Reserved
 *
 *    This source file is subject to the terms and conditions of the
 *    Zensys Software License Agreement which restricts the manner
 *    in which it may be used.
 *
 *---------------------------------------------------------------------------
 *
 * Description: Z-Wave security AES module. Contains the functionality for
 *              implementing secure application communication using AES
 *              as encrypting/decrypting mechanism.
 *              Based on Cryptomatic Security spec and on JRMs C# OFB,
 *              ECB and CBCMAC implementation. Uses IAIK AES128
 *
 * Author:   Johann Sigfredsson, Oleg Zadorozhnyy
 *
 * Last Changed By:  $Author: oza $

 * Revision:         $Revision: 1.11 $
 * Last Changed:     $Date: 2008/09/02 12:39:20 $
 *
 ****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <AES_module.h>
#include <ZW_tx_mutex.h>
#include <ZW_TransportSecurity.h>
#include <ZW_Security_FSM.h>



#ifdef ZW_DEBUG_SECURITY
#define ZW_DEBUG_SECURITY_SEND_BYTE(data) ZW_DEBUG_SEND_BYTE(data)
#define ZW_DEBUG_SECURITY_SEND_STR(STR) ZW_DEBUG_SEND_STR(STR)
#define ZW_DEBUG_SECURITY_SEND_NUM(data)  ZW_DEBUG_SEND_NUM(data)
#define ZW_DEBUG_SECURITY_SEND_WORD_NUM(data) ZW_DEBUG_SEND_WORD_NUM(data)
#define ZW_DEBUG_SECURITY_SEND_NL()  ZW_DEBUG_SEND_NL()
#else
#define ZW_DEBUG_SECURITY_SEND_BYTE(data)
#define ZW_DEBUG_SECURITY_SEND_STR(STR)
#define ZW_DEBUG_SECURITY_SEND_NUM(data)
#define ZW_DEBUG_SECURITY_SEND_WORD_NUM(data)
#define ZW_DEBUG_SECURITY_SEND_NL()
#endif

//TO# 03329   Security smaple code doesn't wait for nonce_get finish

#define NONCEGET_TIMER_TIMEOUT 4

#define INVALID_NODE_ID 0xFF

#define ZCB_BYTE(func) void func(BYTE); \
  code const void (code * func ## _p)(BYTE) = &func; \
  void func

#define ZCB_BYTE_PTR(func) void func(BYTE, void *psTxResult); \
  code const void (code * func ## _p)(BYTE, void *psTxResult) = &func; \
  void func


BOOL SecurityDelayedNonceReport = FALSE;               //we received  NonceReport before callback
BOOL SecurityNonceGetInProgress = FALSE;               //indicate nonce_get callback is in waiting state
static BYTE Security_NonceGetTimerHandle = 0xFF;     //prevent getting stuck
static char Security_NonceGetTimeOut = 0;

static void Security_StartNonceGetTimer();  //prevent getting stuck
static void Security_StopNonceGetTimer();
static void ZCB_Security_NonceGetTimerCallback();

/* This timer invokes app callback with failed status if
 * we timeout waiting for Nonce Report to arrive. */
BYTE nonceGetTimerCountdownB = NONCE_GET_TIMEOUT_B;
BYTE nonceGetTimerHandleB;
void StartNonceGetTimerB(BYTE timeout);

enum SECURITY_MUTEX {MUTEX_FREE=0, MUTEX_LOCKED};
static enum SECURITY_MUTEX fSecurityMutex;

BYTE peerNodeIdWaitingForNonceReport = INVALID_NODE_ID;
BYTE txOptionsWaitingForNonceReport = 0;

static BOOL nonceIsReceived = FALSE;
#ifndef  AES_ON_CHIP
/*==============================   ZW_AES_ECB   ==============================
**    AES ECB - Electronic CodeBook Mode Block
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
#define ZW_AES_ECB(key, inputDat, outputDat) AES128_Encrypt(inputDat, outputDat, key);

#endif

#define SEQ_BUF_SIZE 100 /* TODO: What is a reasonable size? */

static struct {
  BYTE peerNodeId;
  BYTE sequenceCounter;
  BYTE buffer[SEQ_BUF_SIZE];
  BYTE offset;     /* data size stored in buffer */
} seqData;

/*===============================   AES_OFB   ================================
**    AES OFB Output Feedback Block Mode Encryption/Decryption
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
void
AES_OFB(
  BYTE *bufdata,
  BYTE bufdataLength)
{
  register BYTE i, j;
  register BYTE ivIndex;
  blockIndex = 0;

  memset((BYTE *)plaintext16ByteChunk, 0, 16);
  for (cipherIndex = 0; cipherIndex < bufdataLength; cipherIndex++)
  {
    plaintext16ByteChunk[blockIndex] = *(bufdata + cipherIndex);
    blockIndex++;
    if (blockIndex == 16)
    {
      ZW_AES_ECB(encKey, authData.iv, authData.iv);
      ivIndex = 0;
      for (i = (cipherIndex - 15); i <= cipherIndex; i++)
      {
        //  TO#03067 AES_OFB method fails with payload of 32bytes.
////        bufdata[i] = (BYTE)(plaintext16ByteChunk[i] ^ authData.iv[ivIndex]);
        bufdata[i] = (BYTE)(plaintext16ByteChunk[ivIndex] ^ authData.iv[ivIndex]);
        ivIndex++;
      }
      memset((BYTE *)plaintext16ByteChunk, 0, 16);
      blockIndex = 0;
    }
  }

  if (blockIndex != 0)
  {
    ZW_AES_ECB(encKey, authData.iv, authData.iv);
    ivIndex = 0;
    for (j = 0; j < blockIndex; j++)
    {
      bufdata[cipherIndex - blockIndex + j] = (BYTE)(plaintext16ByteChunk[j] ^ authData.iv[j]);
      ivIndex++;
    }
  }
}

/*==============================   AES_CBCMAC   ==============================
**    AES CBCMAC Cipher Block Chaining Mode MAC - Used in authentication
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
void
AES_CBCMAC(
  BYTE *bufdata,
  BYTE bufdataLength,
  BYTE *MAC)
{
  register BYTE i, j, k;

  // Generate input: [header] . [data]
  memcpy((BYTE *)&inputData[0], (BYTE *) &authData.iv[0], 20);
  memcpy((BYTE *)&inputData[20], bufdata, bufdataLength);
  // Perform initial hashing

  // Build initial input data, pad with 0 if length shorter than 16
  for (i = 0; i < 16; i++)
  {
    if (i >= sizeof(authData) + bufdataLength)
    {
      plaintext16ByteChunk[i] = 0;
    }
    else
    {
      plaintext16ByteChunk[i] = inputData[i];
    }

  }
  ZW_AES_ECB(authKey, &plaintext16ByteChunk[0], MAC);
  memset((BYTE *)plaintext16ByteChunk, 0, 16);

  blockIndex = 0;
  // XOR tempMAC with any left over data and encrypt

  for (cipherIndex = 16; cipherIndex < (sizeof(authData) + bufdataLength); cipherIndex++)
  {
    plaintext16ByteChunk[blockIndex] = inputData[cipherIndex];
    blockIndex++;
    if (blockIndex == 16)
    {
      for (j = 0; j <= 15; j++)
      {
        MAC[j] = (BYTE)(plaintext16ByteChunk[j] ^ MAC[j]);
      }
      memset((BYTE *)plaintext16ByteChunk, 0, 16);
      blockIndex = 0;

      ZW_AES_ECB(authKey, MAC, MAC);
    }
  }

  if (blockIndex != 0)
  {
    for (k = 0; k < 16; k++)
    {
      MAC[k] = (BYTE)(plaintext16ByteChunk[k] ^ MAC[k]);
    }
    ZW_AES_ECB(authKey, MAC, MAC);
  }
}

/*================================   AESRaw   ===============================
**    AES Raw
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
/*
Declaration: void AESRaw(BYTE *pKey, BYTE *pSrc, BYTE *pDest)
Called: When individual 128-bit blocks of data have to be encrypted
Arguments: pKey Pointer to key (input; fixed size 16 bytes)
pSrc Pointer to source data (input; fixed size 16 bytes)
pDest Pointer to destination buffer (output; fixed size
16 bytes)
Return value: None
Global vars: None affected
Task: Encrypts 16 bytes of data at pSrc, using Raw AES and the key at pKey. The
16-byte result is written to pDest.*/
void
AESRaw(
  BYTE *pKey,
  BYTE *pSrc,
  BYTE *pDest)
{
  memcpy(pDest, pSrc, 16);
  ZW_AES_ECB(pKey, pSrc, pDest);
}

/*=============================   EncryptPayload   ============================
**    AES EncryptPayLoad
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
/*****************************************************************************/
/*  Declaration: void EncryptPayload(BYTE *pSrc, BYTE length)                */
/*       Called: When payload needs to be encrypted.                         */
/*    Arguments: pSrc Pointer to payload data to encrypt (input)             */
/*               length Length of payload data to encrypt (in bytes;         */
/*               between 0 and 30 both inclusive)                            */
/* Return value: None                                                        */
/*  Global vars: authData.iv[0..15] Read                                     */
/*               encKey[0..15] Read                                          */
/*               payloadPacket[9..l+8] Write (l is length)                   */
/*         Task: Encrypts dataLength bytes of data from pSrc, using AES-OFB, */
/*               the encryption key encKey and the initialization vector     */
/*               authData.iv. The result is written directly into            */
/*               payloadPacket.                                              */
/*****************************************************************************/
void
EncryptPayload(
  BYTE *pSrc,
  BYTE length)
{
  memcpy(payloadPacket + 10, pSrc, length);
  AES_OFB(payloadPacket + 10, length);
}

/*=============================   DecryptPayload   ============================
**    AES DecryptPayload
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
/*
Declaration: void DecryptPayload(BYTE *pBufData, BYTE length)
Called: When payload needs to be decrypted.
Arguments: pBufData Pointer to data to decrypt (input/output)
length Length of payload data to decrypt (in bytes; can
be zero)
Return value: None
Global vars: authData.iv[0..15] Read
encKey[0..15] Read
Task: Decrypts data at pBufData[0..length-1], using AES-OFB, the encryption key
encKey and the initialization vector authData.iv. The result is written to
pBufData[0..length-1].*/
void
DecryptPayload(
  BYTE *pBufData,
  BYTE length)
{
  AES_OFB(pBufData, length);
}

/*=============================   MakeAuthTag   ==============================
**    AES MakeAuthTag
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
/*
Declaration: void MakeAuthTag()
Called: When authentication tag is needed
Arguments: None
Return value: None
Global vars: authData Read (all 20 bytes)
authKey[0..15] Read
payloadPacket[9..p+8] Read - p is
authData.payloadLength)
payloadPacket[p+10..p+17] Write - p is
authData.payloadLength)
Task: Computes the authentication tag for the packet and its header information, using
AES-CBCMAC, the key authKey and the initialization vector
authData.iv. The result is written directly into the payload packet.*/
void
MakeAuthTag(void)
{
  /* AES_CBCMAC calculates 16 byte blocks, we only need 8
     for the auth tag */
  AES_CBCMAC(&payloadPacket[10], authData.payloadLength, tag );
  memcpy (payloadPacket + authData.payloadLength + 11, tag, 8);
}

/*===========================   VerifyAuthTag   ==============================
**    VerifyAuthTag
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
/*
Declaration: BOOL VerifyAuthTag(BYTE *pPayload, BYTE payloadLength)
Called: When an authentication tag is to be verified
Arguments: None
Return value: True if the package was authentic, otherwise false
Global vars: authData Read (all 20 bytes)
authKey[0..15] Read
Temp vars: BYTE tag[8]
Task: Computes the authentication tag for the packet and its header information, using
AES-CBCMAC, the key authKey and the initialization vector
authData.iv. The result is compared to the authentication tag in the payload
packet.
*/
BOOL
VerifyAuthTag(
  BYTE *pPayload,
  BYTE payloadLength)
{
  AES_CBCMAC(pPayload, payloadLength, tag);
  return !memcmp(tag, pPayload + payloadLength + 1, 8);
}

/*===============================   LoadKeys   ==============================
**    LoadKeys
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
/*
Declaration: void LoadKeys()
Called: By system to generate encryption key and authentication key either at boot or
when key in NVRAM has been updated
Arguments: None
Return value: None
Global vars: networkKey[0..15] Read - from NVRAM
authKey[0..15] Write
encKey[0..15] Write
Temp vars: BYTE pattern[16]
Task: Generate authentication and encryption keys from network key.
*/
void
LoadKeys(void)
{
  memset((BYTE *)pattern, 0x55, 16);
  AESRaw(networkKey, pattern, authKey);   /* K_A = AES(K_N, pattern) */
  memset((BYTE *)pattern, 0xAA, 16);
  AESRaw(networkKey, pattern, encKey);    /* K_E = AES(K_N, pattern) */
}

  void PRNGUpdate(void);

/*=============================   PRNGInit   =================================
**    PRNGInit
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
/*
Declaration: void PRNGInit()
Called: When node is powered up
Arguments: None
Return value: None
Global vars: prngState[0..15] Write
Temp vars: None
Task: Initialize PRNG State
*/
void
PRNGInit(void)
{
  /* Reset PRNG State */
  memset(prngState, 0, 16);
  /* Update PRNG State */
  PRNGUpdate();
}

/*===============================   GetRNGData   =============================
**    GetRNGData
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
void
GetRNGData(
  BYTE *pRNDData,
  BYTE noRNDDataBytes)
{
  ZW_SetRFReceiveMode(FALSE);
  i = 0;
  do
  {
#ifdef ZW030x
    ZW_GetRandomWord((BYTE *) (pRNDData + i), FALSE);
#else
#if defined(ZW040x) || defined(ZW050x)
    ZW_GetRandomWord((BYTE *) (pRNDData + i));
#else
    ZW_GetRandomWord((BYTE *) (pRNDData + i), FALSE);
#endif
#endif
    i += 2;
  } while (--noRNDDataBytes && --noRNDDataBytes);
  /* Do we need to reenable RF? */
}

/*==============================   PRNGUpdate   ==============================
**    PRNGUpdate
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
/*
Declaration: void PRNGUpdate()
Called: When fresh input from hardware RNG is needed
Arguments: None
Return value: None
Global vars: prngState[0..15] Modify
Temp data: BYTE k[16], h[16], ltemp[16], i, j
Task: Incorporate new data from hardware RNG into the PRNG State
*/
void
PRNGUpdate(void)
{
  /* H = 0xA5 (repeated x16) */
  memset((BYTE *)h, 0xA5, 16);
  /* The two iterations of the hardware generator */
  for (j = 0; j <= 1; j++)
  {
    /* Random data to K */
    GetRNGData(k, 16);
    /* ltemp = AES(K, H) */
    AESRaw(k, h, ltemp);
    /* H = AES(K, H) ^ H */
    for (i = 0; i <= 15; i++)
    {
      h[i] ^= ltemp[i];
    }
  }
  /* Update inner state */
  /* S = S ^ H */
  for (i = 0; i <= 15; i++)
  {
    prngState[i] ^= h[i];
  }
  /* ltemp = 0x36 (repeated x16) */
  memset((BYTE *)ltemp, 0x36, 16);
  /* S = AES(S, ltemp) */
  AESRaw(prngState, ltemp, btemp);
  memcpy(prngState, btemp, 16);
  /* Reenable RF */
  ZW_SetRFReceiveMode(TRUE);
}

/*=============================   PRNGOutput   ===============================
**    PRNGOutput
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
/*
Declaration: void PRNGOutput(BYTE *pDest)
Called: When 8 bytes of output are needed.
Arguments: pDest Pointer to output data (output; always 8 bytes)
Return value: none
Global vars: prngState[0..15] Modify
Temp data: BYTE ltemp[16]
Task: Generate pseudo-random output data and update PRNG state
*/
void
PRNGOutput(
  BYTE *pDest)
{
  /* Generate output */
  /* ltemp = 0x5C (repeated x16) */
  memset((BYTE *)ltemp, 0x5C/*0xA5*/, 16);
  /* ltemp = AES(PRNGState, ltemp) */
  AESRaw(prngState, ltemp, btemp);
  /* pDest[0..7] = ltemp[0..7] */
  memcpy(pDest, btemp, 8);
  /* Generate next internal state */
  /* ltemp = 0x36 (repeated x16) */
  memset((BYTE *)ltemp, 0x36, 16);
  /* PRNGState = AES(PRNGState, ltemp) */
  AESRaw(prngState, ltemp, btemp);
  memcpy(prngState, btemp, 16);
}

  void ZCB_NonceTimerService(void);
  void InitSecureWakeUp(void);

/*=============================   InitSecurePowerUp   ========================
**    InitSecurePowerUp
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
/*
Declaration: void InitSecurePowerUp()
Called: On power-up or reset
Task: Reset internal nonce table, external nonce record, and nonce request record
(mark all as vacant) and register timer service
Arguments: None
Return value: None
Global vars: None
ltemp data: None
*/
void
InitSecurePowerUp(void)
{
  /* Reset nonce tables */
  InitSecureWakeUp();
  /* Register timer service (100 ms, forever) */
  if(!NonceTimerServiceHanler)
    NonceTimerServiceHanler = TimerStart(ZCB_NonceTimerService, 10, TIMER_FOREVER);
}

/*===========================   InitSecureWakeUp   ===========================
**    InitSecureWakeUp - Reset internal nonce table, external nonce record,
**    and nonce request record (mark all as vacant).
**
**    Side effects
**
**--------------------------------------------------------------------------*/
/*
Declaration: void InitSecureWakeUp()
Called: On wake-up
Task: Reset internal nonce table, external nonce record, and nonce request record
(mark all as vacant).
Arguments: None
Return value: None
Global vars: intNonce[i].nonce[0] Written
enNodeID Written
nrNodeID Written
noncePacket[1] Written
Temp data: BYTE i
*/
void
InitSecureWakeUp(void)
{
  // Reset internal nonce table
  for (i = 0; i < IN_TABLE_SIZE; i++)
  {
    intNonce[i].nonce[0] = 0;
  }
  // Reset external nonce record
  enNodeID = ILLEGAL_NODE_ID;
  // Reset nonce request record
  nrNodeID = ILLEGAL_NODE_ID;
  // Reset noncePacket (used in the message processing module)
  isNonceValid = 0;
  peerNodeIdWaitingForNonceReport = INVALID_NODE_ID;
}

code const void (code * ZCB_NonceTimerService_p)(void) = &ZCB_NonceTimerService;
/*============================   NonceTimerService   =========================
**    NonceTimerService
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
/*
Declaration: void NonceTimerService()
Called: Every 100 ms
Task: Update lifeLeft in nonce table and records and mark timed-out records as
vacant.
Arguments: None
Return value: None
Global vars: intNonce[i].lifeLeft Modified
intNonce[i].nonce[0] May be written
enLifeLeft Modified
enNodeID May be written
nrLifeLeft Modified
nrNodeID May be read, may be written
nrCompletedFunc May be read
Temp data: BYTE i
*/
void
ZCB_NonceTimerService(void)
{
  /* Update internal nonce table */

  for (i = 0; i < IN_TABLE_SIZE; i++)
  {
    /* If timed out... */
    if (intNonce[i].lifeLeft == 0)
    {
      /* Mark as vacant */
      intNonce[i].nonce[0] = 0;
    }
    /* Decrease life left counter */
    intNonce[i].lifeLeft--;
  }
}

/*=================================   MakeIN   ===============================
**    MakeIN
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
/*
Declaration: void MakeIN(BYTE nodeID, BYTE *pDest)
Called: When a new internal nonce is needed
Arguments: nodeID Node ID of destination node
pDest Where to put the internal nonce (output; always
8 bytes)
Return value: None
Global vars: intNonce[i].lifeLeft May be read and/or written
intNonce[i].nonce[j] May be read and/or written
intNonce[i].nodeId Written
authData.iv[0..7] Written
Task: Find a vacant record in the intNonce[] table, generate a nonce and store it
there, copy generated nonce to the first half of the IV (but not to the packet)
Temp data: BYTE i, leastLifeLeft, newIndex
*/
void
MakeIN(
  BYTE tnodeID,
  BYTE *pDest)
{
  ZW_DEBUG_SECURITY_SEND_STR("MakeIN ");
  /* Find record in internal nonce table to use */
  leastLifeLeft = 255;
  for (i = 0; i < IN_TABLE_SIZE; i++)
  {
    /* If vacant... */
    if (intNonce[i].nonce[0] == 0)
    {
      newIndex = i;           /* Choose it */
      break;                  /* And we are done */
    }
    /* If less life left... */
    if (intNonce[i].lifeLeft < leastLifeLeft)
    {
      leastLifeLeft = intNonce[i].lifeLeft; /* Store new life left */
      newIndex = i;           /* And safe index as best bet */
    }
  }
  /* Generate nonce */
  /* Avoid collision check vs. old value */
  intNonce[newIndex].nonce[0] = 0;
  do
  {
    /* Generate new nonce */
    PRNGOutput(&authData.iv[0]);
    for (i = 0; i < IN_TABLE_SIZE; i++)
    {
      /* If collision... */
      if (authData.iv[0] == intNonce[i].nonce[0])
      {
        /* Invalidate */
        authData.iv[0] = 0;
        break;
      }
    }
  }
  while (!authData.iv[0]); /* Until valid nonce is found */
  /* Update intNonce[newIndex] and copy to pDest */
  /* Set life left */
  intNonce[newIndex].lifeLeft = INTERNAL_NONCE_LIFE;
  /* Set nodeID */
  intNonce[newIndex].nodeID = tnodeID;
  /* Copy nonce to nonce table */
  memcpy(intNonce[newIndex].nonce, authData.iv, 8);
  /* Copy nonce to destination */
  memcpy(pDest, authData.iv, 8);
  ZW_DEBUG_SECURITY_SEND_NUM(authData.iv[0]);
  ZW_DEBUG_SECURITY_SEND_NL();
}

/*================================   GetIN   =================================
**    GetIN
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
/*
Declaration: BOOL GetIN(BYTE ri)
Called: When a packet has been received and the associated internal nonce is needed
Arguments: ri: Receiver’s nonce ID of received packet
Return value: True on success, false if RI was not found
Global vars: intNonce[i].nonce[j] May be read
intNonce[i].nonce[0] May be written
intNonce[i].nodeID May be read
authData.iv[8..15] May be written
Task: Find the nonce, copy it to the second half of IV, and mark its record as vacant.
Return true if success and false if the nonce was not found.
Temp data: BYTE i, j
*/
BOOL
GetIN(
  BYTE ri)
{
  ZW_DEBUG_SECURITY_SEND_STR("GetIN ");
  ZW_DEBUG_SECURITY_SEND_NUM(ri);
  /* ri = 0 is not allowed */
  if (ri == 0)
  {
    return FALSE;
  }
  /* Find record */
  for (i = 0; i < IN_TABLE_SIZE; i++) /* For all records in the table... */
  {
    /* If ri found... */
    if (intNonce[i].nonce[0] == ri)
    {
      /* Copy to second half of IV */
      memcpy(authData.iv + 8, intNonce[i].nonce, 8);
      /* Mark all records IN TABLE with same nodeID as vacant */
      for (j = 0; j < IN_TABLE_SIZE; j++)
      {
        /* If same node id.. */
        if (intNonce[j].nodeID == intNonce[i].nodeID)
        {
          /* Mark as vacant */
          intNonce[j].nonce[0] = 0;
        }
      }
      /* Return with success */
      return TRUE;
    }
  }
  /* Return false (ri not found) */
  return FALSE;
}

/*=============================   SendDataSecure   ============================
**
**    Internal SendDataSecure function. Unlike the public ZW_SendDataSecure()
**    this function is not mutex protected.
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
/*
Declaration: BYTE SendDataSecure(BYTE tnodeID, BYTE *pBufData,
  BYTE dataLength, BYTE txSecOptions,
  VOID_CALLBACKFUNC(completedFunc)(BYTE))
Called: Called from the security module to send a nonce request or secure payload.
Arguments: tnodeID Node ID of receiver
pBufData Pointer to data to send
dataLength Length of data to send in bytes - max 30
txSecOptions Transmission security options
completedFunc Callback function to report back to application
Return value: TRUE on success, FALSE if error.
Global vars: nrNodeID May be read, may be written
nrpBufData May be written
nrDataLength May be written
nrLifeLeft May be written
nrTXSecOptions May be written
nrCompletedFunc May be written
authData May be written - some or all variables
payloadPacket May be written
Task: Security layer processing of outgoing packet, i.e. check if too long, get external
nonce, write header, encrypt, authenticate, and send.
Temp data: BYTE ri
Rules:
• The data buffer in the application must not be changed before completeFunc callback is called
• SendDataSecure does not support Ack. If this features is required, the receiving
application should send an acknowledge packet manually.
• It is suggested not to include the txOptions flags in this version as some flags are not allowed
and/or relevant.
txSecOptions flags:
• SEC_REQ_NONCE (0x40): Tell receiver that we would like to get a new nonce back immediately
 - for streaming data
Note: OK to call ZW_SendData() for sending nonce request without checking if ready? probably yes
since it replaces a subsequent call to ZW_SendData
*/
static BYTE
SendDataSecure(
  BYTE tnodeID,
  BYTE *pBufData,
  BYTE dataLength,
  BYTE txSecOptions,
  VOID_CALLBACKFUNC(completedFunc)(BYTE))
{
#ifdef ZW_CONTROLLER
  BYTE NetTmp[16];
#endif
  BYTE retVal;
  UNUSED(completedFunc);
  if(!nodeSecure) {
    ZW_DEBUG_SECURITY_SEND_STR("SendDataSecure: not secure node\r\n");
    return FALSE;
  }
  /* Return error if dataLength > 30 */
  if (dataLength > 30)
  {
    ZW_DEBUG_SECURITY_SEND_STR("SendDataSecure: too much data\r\n");
    return FALSE;
  }
  if (IS_NULL(pBufData))
  {
    ZW_DEBUG_SECURITY_SEND_STR("SendDataSecure: buf is NULL\r\n");
    return FALSE;
  }
  /* Get external nonce (or request one) */
  /* Get external nonce and RI */
  if (tnodeID != enNodeID)
  {
    /* If ENR buffer in use... */
    /* Save needed data in NR record */
    nrpBufData = pBufData;
    nrDataLength = dataLength;
    nrNodeID = tnodeID;
    nrTXSecOptions = txSecOptions;
    /* Send nonce request */
    StartSecuritySendTimeOut(NONCE_TIMER);
    //TO# 3445
    StartSecuritySendDataTimer(SECURITY_SEND_DATA_TIME_OUT);

    nonceRequestPacket[0] = COMMAND_CLASS_SECURITY;
    nonceRequestPacket[1] = SECURITY_NONCE_GET;
    //TO# 03329
    SecurityNonceGetInProgress = TRUE;  // Nonce Get In Progress
    SecurityDelayedNonceReport = FALSE;
    Security_StartNonceGetTimer();
    ZW_DEBUG_SECURITY_SEND_BYTE('G');

#ifdef ZW_CONTROLLER
    return (ZW_SendData(tnodeID, nonceRequestPacket, 2, txSecOptions | TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_EXPLORE, &ZCB_NonceRequestCompleted));
#else
    retVal = ZW_SendData(tnodeID, nonceRequestPacket, 2, txSecOptions, &ZCB_NonceRequestCompleted);
    if (retVal)
    {
      PostEvent(EV_NONCE_GET_TRANSMITTING);
    }
    else
    {
      ZCB_NonceRequestCompleted(TRANSMIT_COMPLETE_NO_ACK, NULL);
    }
    return retVal;
#endif
  }
  else
  {
    /* Copy the nonce to second half of IV */
    memcpy(authData.iv + 8, enNonce, 8);
    /* Mark the nonce buffer as empty */
    enNodeID = ILLEGAL_NODE_ID;
  }

  /* Write header (SH) */
  /* bitwise OR */
#ifdef ZW_CONTROLLER
  if (OutKeyNewController)
  {
    memcpy(NetTmp,networkKey,16);
    memcpy(networkKey, inclusionKey, 16);
    LoadKeys();
  }
#endif

  StopSecuritySendTimeOut();


  memcpy(outPayload+1,pBufData,dataLength);
  outPayload[0] = 0;
  pBufData = outPayload;
  dataLength++;
  authData.sh = 0x81;
  payloadPacket[0] = COMMAND_CLASS_SECURITY;
  payloadPacket[1] = authData.sh;
  /* Write sender’s nonce (SN) */
  MakeIN(tnodeID, payloadPacket + 2);
  /* Encrypt payload if present (EP) */
  EncryptPayload(pBufData, dataLength);
  /* Write receiver’s nonce identifier (RI) */
  payloadPacket[dataLength + 10] = enNonce[0];
  /* Generate authentication tag (AT) */
  authData.senderNodeID = nodeID;
  authData.receiverNodeID = tnodeID;
  authData.payloadLength = dataLength;
  memcpy(&authData.iv[0],&payloadPacket[2] ,8);
  memcpy(&authData.iv[8],enNonce,8);
  /* Add AT */
  MakeAuthTag();
  /* Send data */
#ifdef ZW_CONTROLLER
  if (OutKeyNewController)
  {
    OutKeyNewController = 0;
    memcpy(networkKey,NetTmp,16);
    LoadKeys();
  }
  return ZW_SendData(tnodeID, payloadPacket, dataLength+19, nrTXSecOptions | TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_EXPLORE, completedFunc);
#else
  retVal =  ZW_SendData(tnodeID, payloadPacket, dataLength+19, nrTXSecOptions, ZCB_SecureSessionDone);
  if (!retVal)
  {
    FreeSecurityMutex();
    PostEvent(EV_RETURN_TO_IDLE);
  }
  return retVal;
#endif
}

/*=============================   ZW_SendDataSecure   ============================
**
**    Public function to send a secure message. Called from application layer.
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
/*
Called: Called from application to send a secure packet with payload
Arguments: tnodeID Node ID of receiver
pBufData Pointer to data to send
dataLength Length of data to send in bytes - max 30
txSecOptions Transmission security options
completedFunc Callback function to report back to application
Return value: TRUE on success, FALSE if error.
Global vars: nrNodeID May be read, may be written
nrpBufData May be written
nrDataLength May be written
nrLifeLeft May be written
nrTXSecOptions May be written
nrCompletedFunc May be written
authData May be written - some or all variables
payloadPacket May be written
Task: Security layer processing of outgoing packet, i.e. check if too long, get external
nonce, write header, encrypt, authenticate, and send.
Temp data: BYTE ri
Rules:
• The data buffer in the application must not be changed before completeFunc callback is called
• SendDataSecure does not support Ack. If this features is required, the receiving
application should send an acknowledge packet manually.
• It is suggested not to include the txOptions flags in this version as some flags are not allowed
and/or relevant.
txSecOptions flags:
• SEC_REQ_NONCE (0x40): Tell receiver that we would like to get a new nonce back immediately
 - for streaming data
Note: OK to call ZW_SendData() for sending nonce request without checking if ready? probably yes
since it replaces a subsequent call to ZW_SendData
*/
BYTE
ZW_SendDataSecure(
  BYTE tnodeID,
  BYTE *pBufData,
  BYTE dataLength,
  BYTE txSecOptions,
  VOID_CALLBACKFUNC(completedFunc)(BYTE))
{
  BYTE retVal;
  if(FALSE ==GetSecurityMutex())
  {
    ZW_DEBUG_SECURITY_SEND_STR("Security Mutex BUSY\r\n");
    return FALSE;
  }
  nrCompletedFunc = completedFunc;
  ZW_DEBUG_SECURITY_SEND_STR("\r\nZW_SendDataSecure");
  ZW_DEBUG_SECURITY_SEND_NUM(tnodeID);
  ZW_DEBUG_SECURITY_SEND_NUM(*pBufData);
  ZW_DEBUG_SECURITY_SEND_NL();
  retVal = SendDataSecure(tnodeID, pBufData, dataLength, txSecOptions, completedFunc);
  if (!retVal)
  {
    ZW_DEBUG_SECURITY_SEND_STR("ZW_SendDataSecure returned false\r\n");
    FreeSecurityMutex();
  }
  return retVal;
}

/* Memcpy that checks size of dest buffer before writing */
void memcpy_s(
  BYTE *dst,      /* IN   Pointer to destination */
  BYTE *src,      /* IN   Pointer to source */
  BYTE length,    /* IN   Number of bytes to copy */
  BYTE bufSize)  /* IN   Size of destination buffer*/
{
  if (length > bufSize)
  {
    length = bufSize;
  }
  memcpy(dst, src, length);
}

#define IS_FIRST_FRAME(h) (((h) & SECURITY_MESSAGE_ENCAPSULATION_PROPERTIES1_SEQUENCED_BIT_MASK) \
            && !((h) & SECURITY_MESSAGE_ENCAPSULATION_PROPERTIES1_SECOND_FRAME_BIT_MASK))

#define IS_SEQUENCED(h) ((h) && SECURITY_MESSAGE_ENCAPSULATION_PROPERTIES1_SEQUENCED_BIT_MASK)

/*=========================   ProcessIncomingSecure   ========================
**    ProcessIncomingSecure
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
/*
Declaration: BYTE ProcessIncomingSecure(BYTE tnodeID, BYTE *pPacket,
BYTE packetSize)
Called: When a packet with Command Class COMMAND_CLASS_SECURITYsecurity flag has been received.
Arguments: pPacket Pointer to packet data
packetSize Size of packet
tnodeID Node ID of sender
Return value: Size of decrypted data including service byte to be passed
on to application layer (-1 if the packet did
not contain any payload; zero if it contained an empty payload)
Global vars: authData May be written (some or all variables)
noncePacket May be written
Task: Process security fields in packet and decrypt payload data if present. Decrypted
data, if any, is put in the pPacket buffer starting from address 9. Send nonce
packet if requested.
Temp data: None
Note: OK to call ZW_SendData() without checking if ready?
*/
BYTE                        /* RET  Size of decrypted payload including seq control byte a.k.a. service byte */
ProcessIncomingSecure(
  BYTE tnodeID,
  BYTE *pPacket,            /* First byte is security header */
  BYTE txSecOptions,
  BYTE packetSize)
{
  /* Macro to convert packetSize to size of encapsulated payload */
  /* 19 corresponds to security header (1), IV (8), sequencing hdr (1), r-nonce id (1), and MAC (8) */
  #define PAYLOAD_SIZE(p) ((p) - 19)
  BYTE seqHeader;   /* sequencing header */
  BYTE reassembled_frame_size;
  BYTE internal_nonce_found;

  /* If no packet data... */
  StopSecuritySendTimeOut();
  if (packetSize < 1)
  {
    /* Done (not even a header present) */
    return 0xff;
  }
  /* Extract security header */
  authData.sh = pPacket[0];
  ZW_DEBUG_SECURITY_SEND_STR("sh: ");
  ZW_DEBUG_SECURITY_SEND_NUM(authData.sh);
  ZW_DEBUG_SECURITY_SEND_NL();
  /* If any illegal flags are set... */
  if ((authData.sh & MASK_SECURITY_HEADER_INVALID) != 0)
  {
    /* Discard */
    return 0xff;
  }
  /* if we need a nonce, extract it now before making a new one */
  /* otherwise GetIN() after MakeIN() will invalidate the new IN */
  if (authData.sh == SECURITY_MESSAGE_ENCAPSULATION_NONCE_GET ||
      authData.sh == SECURITY_MESSAGE_ENCAPSULATION)
  {
    internal_nonce_found = GetIN(pPacket[packetSize - 9]);
  }

  /* Send nonce if Nonce Request flag is set */
  if ((authData.sh & MASK_SECURITY_HEADER_NONCE_REQEUST) != 0)
  {
    if (ST_IDLE == GetSecurityFsmState())
    {
      /* If NR flag is set and noncePacket is */
      /* vacant... */
      notSleep = 0;
      noncePacket[0] = COMMAND_CLASS_SECURITY;
      /* Set nonce packet header */
      noncePacket[1] = SECURITY_NONCE_REPORT;
      /* Generate nonce */
      MakeIN(tnodeID, noncePacket + 2);
      isNonceValid = 1;
      ZW_DEBUG_SECURITY_SEND_STR("Tx nonce: ");
      ZW_DEBUG_SECURITY_SEND_NUM(noncePacket[2]);
      ZW_DEBUG_SECURITY_SEND_NUM(noncePacket[3]);
      ZW_DEBUG_SECURITY_SEND_NUM(noncePacket[4]);
      ZW_DEBUG_SECURITY_SEND_NL();
      /* Send packet */
  #ifdef ZW_CONTROLLER
      ZW_SendData(tnodeID, noncePacket, 10, txSecOptions, ZCB_NonceCompleted);
  #else
  //         ZW_SendData(tnodeID, noncePacket, 10, txSecOptions, ZCB_NonceCompleted);
      if (!ZW_SendData(tnodeID, noncePacket, 10, txSecOptions, ZCB_NonceCompleted))
      {
        ZCB_NonceCompleted(TRANSMIT_COMPLETE_FAIL, NULL);
        ZW_DEBUG_SECURITY_SEND_STR("\r\nNR Tx fail\r\n");
      }
      else
      {
        PostEvent(EV_NONCE_REPORT_TRANSMITTING);
      }
  #endif
      StartSecuritySendTimeOut(NONCE_TIMER);
    }
    else
    {
      if (ST_SENDING_NONCE_REPORT == GetSecurityFsmState())
      {
        ZW_DEBUG_SECURITY_SEND_STR(" Ignore nonce get ");
        /* Avoid queuing 2 nonce reports, we might run out of
           Tx buffers if we do */
        return 0xff;
      }
      /* Hold back transmit of Nonce report until FSM returns
       * to idle */
      peerNodeIdWaitingForNonceReport = tnodeID;
      txOptionsWaitingForNonceReport = txSecOptions;
    }
  }
#if 0
  /* Disabled because using IV of previous encap as nonce for reply
   * is not backward compatible. Cannot fix this until time travel
   * has been perfected. */
  // Extract sender’s nonce if present
  if ((authData.sh & MASK_SECURITY_HEADER_NONCE_PRESENT) != 0)
  {
    ZW_DEBUG_SECURITY_SEND_STR("Rx nonce: ");
    ZW_DEBUG_SECURITY_SEND_NUM(pPacket[1]);
    ZW_DEBUG_SECURITY_SEND_NUM(pPacket[2]);
    ZW_DEBUG_SECURITY_SEND_NUM(pPacket[3]);
    ZW_DEBUG_SECURITY_SEND_NL();
    RegisterEN(tnodeID, pPacket + 1);
  }
#endif
  /* All packets except nonce and nonce req. (which has been */
  /* processed already) has authentication tag */
  /* If too small for auth tag... */
  if (packetSize < 18)
  {
    /* Discard (incomplete packet) */
    return 0xff;
  }
  /* Copy sender’s nonce to IV */
  memcpy(authData.iv, pPacket + 1, 8);
  /* Find the internal nonce */
  /* If internal nonce is not found... check nonce identifier if present */
  if (!internal_nonce_found)
  {
    ZW_DEBUG_SECURITY_SEND_BYTE('&');
    /* Discard (fake or expired RI) */
    return 0xff;
  }
  /* Verify authentication */
  authData.senderNodeID = tnodeID;
  authData.receiverNodeID = nodeID;
  authData.payloadLength = packetSize - 18;
  /* If not authentic... */
  if (!VerifyAuthTag(pPacket + 9, packetSize - 18))
  {
    /* Discard (wrong auth. tag) */
    ZW_DEBUG_SECURITY_SEND_STR("Auth bad\r\n");
    return 0xff;
  }
  ZW_DEBUG_SECURITY_SEND_STR("Auth ok\r\n");
  /* Process message if present */
  /* If contains message... */
  if ((authData.sh & ~MASK_SECURITY_HEADER_NONCE_REQEUST) == SECURITY_MESSAGE_ENCAPSULATION)
  {
    /* Decrypt it */
    DecryptPayload(pPacket + 9, packetSize - 18);
    /* PostEvent must come after DecryptPayload, because DecryptPayload()
     *  can mess with global authData */
    PostEvent(EV_SEC_ENCAP_ARRIVES);
    seqHeader = pPacket[9];
    ZW_DEBUG_SECURITY_SEND_STR("seqHeader: ");
    ZW_DEBUG_SECURITY_SEND_NUM(seqHeader);
    ZW_DEBUG_SECURITY_SEND_NL();
    if (!IS_SEQUENCED(seqHeader))
    {
      /* This function returns payload length INCLUDING seq header*/
      return PAYLOAD_SIZE(packetSize) + 1;
    }
    else
    {
      ZW_DEBUG_SECURITY_SEND_STR("IS_SEQUENCED\r\n");
      if (IS_FIRST_FRAME(seqHeader))
      {
        seqData.peerNodeId = tnodeID;
        seqData.sequenceCounter = seqHeader & SECURITY_MESSAGE_ENCAPSULATION_PROPERTIES1_SEQUENCE_COUNTER_MASK;
        seqData.offset = PAYLOAD_SIZE(packetSize);
        ZW_DEBUG_SECURITY_SEND_STR("IS_FIRST. Offset is ");
        ZW_DEBUG_SECURITY_SEND_NUM(seqData.offset);
        ZW_DEBUG_SECURITY_SEND_NL();
        memcpy_s(seqData.buffer, &pPacket[10], seqData.offset, sizeof(seqData.buffer)); /* 10 includes security header, IV and seq header byte */
      }
      else
      {
        /* this is second part of sequenced frame*/
        ZW_DEBUG_SECURITY_SEND_STR("IS_SECOND. payload size is ");
        ZW_DEBUG_SECURITY_SEND_NUM(PAYLOAD_SIZE(packetSize));
        ZW_DEBUG_SECURITY_SEND_NL();
        if (seqData.peerNodeId == tnodeID
            && seqData.sequenceCounter == (seqHeader & SECURITY_MESSAGE_ENCAPSULATION_PROPERTIES1_SEQUENCE_COUNTER_MASK))
        {
          ZW_DEBUG_SECURITY_SEND_STR("Filter match");
          if (seqData.offset < PAYLOAD_SIZE(packetSize))
          {
            /* overlapping memcpy regions - abort */
            ZW_DEBUG_SECURITY_SEND_STR("Too short first fragment");
            seqData.peerNodeId = INVALID_NODE_ID;
            return 0xff;
          }
          /* move second frame payload data to make room for first frame payload */
          memcpy(&pPacket[10 + seqData.offset], &pPacket[10], PAYLOAD_SIZE(packetSize));
          /* copy back first frame payload */
          memcpy(&pPacket[10], seqData.buffer, seqData.offset);
          seqData.peerNodeId = INVALID_NODE_ID;
          reassembled_frame_size = seqData.offset + PAYLOAD_SIZE(packetSize);
          ZW_DEBUG_SECURITY_SEND_STR("Reassembled frame of len ");
          ZW_DEBUG_SECURITY_SEND_NUM(reassembled_frame_size);
          ZW_DEBUG_SECURITY_SEND_NL();
          /* Returns payload length INCLUDING seq header*/
          return reassembled_frame_size + 1;
        }
      }
    }
  }
  /* Done; no payload */
  PostEvent(EV_SEC_ENCAP_ARRIVES);
  return 0xff;
  #undef PAYLOAD_SIZE
}

code const void (code * ZCB_NonceCompleted_p)(BYTE txStatus, void *psTxResult) = &ZCB_NonceCompleted;
/*=============================   NonceCompleted   ============================
**    NonceCompleted
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
/*
Declaration: void NonceCompleted(BYTE txStatus)
Called: By lower protocol level when a nonce report packet has been sent.
Arguments: txStatus If transmission was successful.
Return value: None
Global vars: noncePacket[2] Written
Task: Mark nonce packet buffer as vacant.
Temp data: None
*/
void
ZCB_NonceCompleted(
  BYTE txStatus,
  void *psTxResult)
{
  UNUSED(psTxResult);
  ZW_DEBUG_SECURITY_SEND_STR("ZCB_NonceCompleted\r\n");
  if (TRANSMIT_COMPLETE_OK == txStatus)
  {
    PostEvent(EV_NONCE_REPORT_TRANSMIT_OK);
  }
  else
  {
    PostEvent(EV_RETURN_TO_IDLE);
  }
  /* Mark nonce packet as vacant */
  isNonceValid = 0;
}


//TO# 03329 Start Timer callback
void
Security_StartNonceGetTimer(
  void)
{
  if (Security_NonceGetTimerHandle != 0xFF)
  {
    ZW_TIMER_CANCEL(Security_NonceGetTimerHandle);
  }
  Security_NonceGetTimerHandle = ZW_TIMER_START(
                                   ZCB_Security_NonceGetTimerCallback,
                                   TIMER_ONE_SECOND,
                                   TIMER_FOREVER);
  Security_NonceGetTimeOut = NONCEGET_TIMER_TIMEOUT;
}

//TO# 03329 Stop Timer callback
void
Security_StopNonceGetTimer(
  void)
{
  if (Security_NonceGetTimerHandle != 0xFF)
  {
    ZW_TIMER_CANCEL(Security_NonceGetTimerHandle);
  }
  Security_NonceGetTimerHandle = 0xFF;
}

code const void (code * ZCB_Security_NonceGetTimerCallback_p)(void) = &ZCB_Security_NonceGetTimerCallback;
//TO# 03329 Timer callback to prevent getting stuck
void
ZCB_Security_NonceGetTimerCallback(
  void)
{
  Security_NonceGetTimeOut--;
  if (Security_NonceGetTimeOut <= 0)
  {
    Security_StopNonceGetTimer();
    SecurityNonceGetInProgress = FALSE;    // timer is expired
    if (SecurityDelayedNonceReport == TRUE)
    {
      //ZW_DEBUG_SECURITY_SEND_STR("ZCB_Security_NonceGetTimerCallback");
      //ZW_DEBUG_SECURITY_SEND_NL();
      // we have delayed n-r
      SecurityDelayedNonceReport = FALSE;
      NonceHasBeenReceived();
    }
  }
}

code const void (code * ZCB_NonceRequestCompleted_p)(BYTE txStatus, void *psTxResult) = &ZCB_NonceRequestCompleted;
/*=========================   NonceRequestCompleted   ========================
**    NonceRequestCompleted
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
/*
Declaration: void NonceRequestCompleted(BYTE txStatus)
Called: By lower protocol level when a nonce request packet has been sent.
Arguments: txStatus If transmission was successful.
Return value: None
Global vars: nrNodeID May be written
nrCompletedFunc May be read
Task: In case of error: Clear nonce request record and forward error to application
Temp data: None
*/
void
ZCB_NonceRequestCompleted(
  BYTE txStatus,
  void *psTxResult)
{
  UNUSED(psTxResult);
  //ZW_DEBUG_SECURITY_SEND_BYTE('g');
  /* Do nothing if everything is OK */
  /* If status is OK... */
  if (txStatus == TRANSMIT_COMPLETE_OK)
  {
    PostEvent(EV_NONCE_GET_TRANSMIT_OK);
    //ZW_DEBUG_SECURITY_SEND_BYTE('g');
    /* Done */
///    StartSecuritySendTimeOut(NONCE_REQUEST_TIMER);
    //TO# 03329
    Security_StopNonceGetTimer();
    SecurityNonceGetInProgress = FALSE;   // nonse_get callback is finished

    if (SecurityDelayedNonceReport == TRUE)
    {
      //ZW_DEBUG_SECURITY_SEND_STR("ZCB_NonceRequestCompleted");
      //ZW_DEBUG_SECURITY_SEND_NL();
      SecurityDelayedNonceReport = FALSE;
      /*Check if mutex still is locked*/
      NonceHasBeenReceived();
    }
    StartNonceGetTimerB(NONCE_GET_TIMEOUT_B);
    return;
  }
  /* Clear nonce request record */
  /* Mark as vacant */
  nrNodeID = ILLEGAL_NODE_ID;
  ZW_DEBUG_SECURITY_SEND_STR("ZCB_NonceRequestCompleted FAIL: ");
  ZW_DEBUG_SECURITY_SEND_NUM(txStatus);
  ZW_DEBUG_SECURITY_SEND_NL();
  ZCB_SecureSessionDone(txStatus, NULL);
}

/*
 *  Timer callback func, see description in StartNonceGetTimerB
 */
void ZCB_NonceGetTimeoutB(void);
code const void (code * ZCB_NonceGetTimeoutB_p)(void) = &ZCB_NonceGetTimeoutB;
void
ZCB_NonceGetTimeoutB()
{
  ZW_DEBUG_SECURITY_SEND_BYTE(' ');
  ZW_DEBUG_SECURITY_SEND_BYTE('B');
  ZW_DEBUG_SECURITY_SEND_NUM(nonceGetTimerCountdownB);
  ZW_DEBUG_SECURITY_SEND_BYTE(' ');

  if (nonceGetTimerCountdownB > 0)
  {
    nonceGetTimerCountdownB--;
    return;
  }
  enNodeID = ILLEGAL_NODE_ID;

  if(nonceGetTimerHandleB!=0xff)
  {
    ZW_TIMER_CANCEL(nonceGetTimerHandleB);
    nonceGetTimerHandleB = 0xff;
  }
  ZW_DEBUG_SECURITY_SEND_BYTE('B');
  ZW_DEBUG_SECURITY_SEND_BYTE('_');
  ZCB_SecureSessionDone(TRANSPORT_SECURITY_TIMEOUT, NULL);
}

void TimerStop(BYTE *handle)
{
  if (*handle != 0xFF)
  {
    ZW_TIMER_CANCEL(*handle);
    *handle = 0xFF;
  }
}


/*
 * This timer runs while waiting for a nonce report. In case the report never comes,
 * a callback is delivered to the application.
 *
 * Note: The Security_StartNonceGetTimer() has a different purpose despite the similar name.
 * AFAIK it handles the situation where the Report comes before the TX Complete callback
 * on the Nonce Request. See TO#3329 for details on this.
 */
void
StartNonceGetTimerB(
  BYTE timeout)      /* IN Timeout in seconds */
{
  nonceGetTimerCountdownB = timeout;
  TimerStop(&nonceGetTimerHandleB);
  nonceGetTimerHandleB = ZW_TIMER_START(ZCB_NonceGetTimeoutB, TIMER_ONE_SECOND, TIMER_FOREVER);
  if (nonceGetTimerHandleB == 0xFF)
  {
    ZW_DEBUG_SECURITY_SEND_STR("Failed to get timer for NonceGetTimerB +++++\r\n");
  }
  ZW_DEBUG_SECURITY_SEND_STR("StartNonceGetTimerB got handle ");
  ZW_DEBUG_SECURITY_SEND_NUM(nonceGetTimerHandleB);
  ZW_DEBUG_SECURITY_SEND_NL();

}

void
StopNonceGetTimerB()
{
  ZW_DEBUG_SECURITY_SEND_STR("TimerB stopped");
  if(nonceGetTimerHandleB!=0xff)
  {
    ZW_DEBUG_SECURITY_SEND_BYTE('.');
    ZW_TIMER_CANCEL(nonceGetTimerHandleB);
    nonceGetTimerHandleB = 0xff;
  }
  ZW_DEBUG_SECURITY_SEND_NL();
}

void ZCB_SecuritySendTimeOut(void);
code const void (code * ZCB_SecuritySendTimeOut_p)(void) = &ZCB_SecuritySendTimeOut;
void
ZCB_SecuritySendTimeOut()
{
  ZW_DEBUG_SECURITY_SEND_BYTE(' ');
  ZW_DEBUG_SECURITY_SEND_BYTE('Z');
  ZW_DEBUG_SECURITY_SEND_NUM(securitySendLife);
  ZW_DEBUG_SECURITY_SEND_BYTE(' ');

  if (securitySendLife==0)
  {
    enNodeID = ILLEGAL_NODE_ID;
    if(securitySendTimerHandle!=0xff)
    {
      ZW_TIMER_CANCEL(securitySendTimerHandle);
      securitySendTimerHandle = 0xff;
    }

#ifdef ZW_CONTROLLER
    nodeSecure = 0;
    nodeSecureIncl = 0;
    if(isCtrlIncluded())
    {
      setCtrlNoneSecure();
    }
    else
    {
      AddSecuritySlave(nodeInWork,FALSE);
    }
#else
    ZW_DEBUG_SECURITY_SEND_BYTE('9');
    ZW_DEBUG_SECURITY_SEND_BYTE('_');
    //Transport_SetNodeSecure(NON_SECURE_NODE);
#endif

    ZCB_FuncZWSecure(TRANSPORT_WORK_ERROR);

    ZW_DEBUG_SECURITY_SEND_BYTE('_');
    ZW_DEBUG_SECURITY_SEND_BYTE('e');
    ZW_DEBUG_SECURITY_SEND_BYTE('e');

    return;
  }
  securitySendLife--;
}

void
StartSecuritySendTimeOut(
  BYTE timeOut)
{
  if (timeOut<3) timeOut = 3;
//t
  if (timeOut > 20) timeOut = 20;

  securitySendLife = timeOut;
  if(securitySendTimerHandle!=0xff)
  {
    ZW_TIMER_CANCEL(securitySendTimerHandle);
    securitySendTimerHandle = 0xff;
  }
  securitySendTimerHandle = ZW_TIMER_START(ZCB_SecuritySendTimeOut, TIMER_ONE_SECOND, TIMER_FOREVER);
  ZCB_FuncZWSecure(TRANSPORT_WORK_START);
}

void
StopSecuritySendTimeOut()
{
  if(securitySendTimerHandle!=0xff)
  {
    ZW_TIMER_CANCEL(securitySendTimerHandle);
    securitySendTimerHandle = 0xff;
  }
  ZCB_FuncZWSecure(TRANSPORT_WORK_END);
}

code const void (code * ZCB_SecurityTimeOut_p)(void) = &ZCB_SecurityTimeOut;
void
ZCB_SecurityTimeOut()
{

  ZW_DEBUG_SECURITY_SEND_BYTE(' ');
  ZW_DEBUG_SECURITY_SEND_BYTE('X');
  ZW_DEBUG_SECURITY_SEND_NUM(securityLife);
  ZW_DEBUG_SECURITY_SEND_BYTE(' ');

  if (securityLife==0)
  {
    if(securityTimerHandle!=0xff)
    {
      ZW_TIMER_CANCEL(securityTimerHandle);
      securityTimerHandle = 0xff;
    }
#ifdef ZW_CONTROLLER
    nodeSecureIncl = 0;
    if(isCtrlIncluded())
    {
      setCtrlNoneSecure();
    }
    else
    {
      AddSecuritySlave(nodeInWork,FALSE);
    }
#else
    Transport_SetNodeSecure(NON_SECURE_NODE);
#endif
    ZCB_FuncZWSecure(TRANSPORT_WORK_ERROR);
    ZW_DEBUG_SECURITY_SEND_BYTE('_');
    ZW_DEBUG_SECURITY_SEND_BYTE('d');
    ZW_DEBUG_SECURITY_SEND_BYTE('d');
    return;
  }
  securityLife--;
}

void
StartSecurityTimeOut(BYTE timeOut)
{
///  if (timeOut<10) timeOut = 10;
//t
  if (timeOut > 20) timeOut = 20;
  securityLife = timeOut;
  if(securityTimerHandle!=0xff)
  {
    ZW_TIMER_CANCEL(securityTimerHandle);
    securityTimerHandle = 0xff;
  }
  securityTimerHandle = ZW_TIMER_START(ZCB_SecurityTimeOut, TIMER_ONE_SECOND, TIMER_FOREVER);

  ZCB_FuncZWSecure(TRANSPORT_WORK_START);

  ZW_DEBUG_SECURITY_SEND_BYTE(' ');
  ZW_DEBUG_SECURITY_SEND_BYTE('M');
  ZW_DEBUG_SECURITY_SEND_NUM(securityTimerHandle);
  ZW_DEBUG_SECURITY_SEND_BYTE(' ');

}

/*==========   StartSecurityTimeoutIfNotRunning   ==========================
**
**  Start security timeout if not already running. The timeout is responsible
**  turning the node nonsecure 10 seconds after lower layer inclusion has
**  finished.
**
**  Side effects: None
**
**-------------------------------------------------------------------------*/
void
StartSecurityTimeoutIfNotRunning(BYTE timeout)
{
  /* These values indicate a reinitialized node
   * or a cancelled timer repectively.
   * Any other value means the timer is running. */
  ZW_DEBUG_SEND_STR("\r\nStartSecurityTimeoutIfNotRunning ->");
  ZW_DEBUG_SEND_NUM(timeout);
  ZW_DEBUG_SEND_NL();
  if (securityTimerHandle == 0x00
      || securityTimerHandle == 0xff)
  {
    StartSecurityTimeOut(timeout);
  }
  else
  {
    ZW_DEBUG_SECURITY_SEND_STR("\r\nSecurityTimerHandle already running - not restarting\r\n");
  }
}

void
StopSecurityTimeOut()
{

  ZW_DEBUG_SECURITY_SEND_BYTE(' ');
  ZW_DEBUG_SECURITY_SEND_BYTE('S');
  ZW_DEBUG_SECURITY_SEND_BYTE(' ');

  if(securityTimerHandle!=0xff)
  {
    ZW_TIMER_CANCEL(securityTimerHandle);
    securityTimerHandle = 0xff;
  }
  ZCB_FuncZWSecure(TRANSPORT_WORK_END);
}

#ifndef  AES_ON_CHIP
/*=============================   AES128_Encrypt   ============================
**    AES128_Encrypt
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
/*******************************************************************************
Declaration: void AES128_Encrypt(const unsigned char *ext_input,
                                 unsigned char *ext_output,const
                                 unsigned char *ext_key )

This is the main routine for Encryption. The C part only handles the parameter
passing to the assembler routines. For this reason the C-routine copies the input data
and the cipherkey to the absolut addresses ASM_input and ASM_key, which are located in
the data segment of the assembler module.
After the call of the assembler part the routine copies the encrypted data from
ASM_input to the memory area to which the generic pointer *ext_output is pointing.
*******************************************************************************/
void
AES128_Encrypt(
  const BYTE *ext_input,
  BYTE *ext_output,
  const BYTE *ext_key)
{
  input_idata = &ASM_input[0];
  key_idata = &ASM_key[0];

  memcpy(input_idata, ext_input, 16);
  memcpy(key_idata, ext_key, 16);
  ASM_AES128_Encrypt();
  memcpy(ext_output, input_idata, 16);
}

/*=============================   AES128_Decrypt   ============================
**    AES128_Decrypta
**
**    Side effects :
**
**--------------------------------------------------------------------------*/
/*******************************************************************************
Declaration: void AES128_Decrypt(const unsigned char *ext_input,
                                 unsigned char *ext_output,
                                 const unsigned char *ext_key)

This is the main routine for Decryption. The C part only handles the parameter
passing to the assembler routines. For this reason the C-routine copies the input data
and the cipherkey to the absolut addresses ASM_input and ASM_key, which are located in
the data segment of the assembler module.
After the call of the assembler part the routine copies the decrypted data from
ASM_input to the memory area to which the generic pointer *ext_output is pointing.
*******************************************************************************/
void
AES128_Decrypt(
  const BYTE *ext_input,
  BYTE *ext_output,
  const BYTE *ext_key)
{

  input_idata = &ASM_input[0];
  key_idata = &ASM_key[0];

  memcpy(input_idata, ext_input, 16);
  memcpy(key_idata, ext_key, 16);
  ASM_AES128_Decrypt();
  memcpy(ext_output, input_idata, 16);
}
#endif
/*==============================   InitSecurity   ============================
**    Initialization of the Security module, can be called in ApplicationInitSW
**
**    This is an application function example
**
**--------------------------------------------------------------------------*/
void
InitSecurity(
  BYTE wakeUpReason)
{
  LoadKeys();
  if (wakeUpReason == ZW_WAKEUP_RESET)
  {
    /* Reset or External Int is the wakeup reason */
    /* Reinitialize Pseudo Random seed, for random generator, */
    /* with Z-Wave TRUE RF random generator */
    PRNGInit();
    /* Init the Security module */
    InitSecurePowerUp();
  }
  else if (wakeUpReason == ZW_WAKEUP_WUT)
  {
    /* WUT timer is the wakeup reason */
    /* Pseudo Random seed for random generator should still */
    /* be valid as placed in NON_ZERO_START_ADDR */
    /* Init the Security module */
    InitSecurePowerUp();
  }
#ifdef ZW_ZENSOR
  else if (wakeUpReason == ZW_WAKEUP_SENSOR)
  {
    /* Pseudo Random seed for random generator should still */
    /* be valid as placed in NON_ZERO_START_ADDR */
    /* Wakeup Beam is the wakeup reason */
    /* Init the Security module */
    InitSecurePowerUp();
  }
#endif
}

void FreeSecurityMutex()
{
  if(fSecurityMutex == MUTEX_FREE)
  {
    ZW_DEBUG_SECURITY_SEND_STR("Warning: Security mutex already free when freed\r\n");
  }
  ZW_DEBUG_SECURITY_SEND_STR("Security mutex freed\n\r");
  fSecurityMutex = MUTEX_FREE;
  nonceIsReceived = FALSE;
}

BYTE IsSecurityMutexLocked()
{
  return fSecurityMutex == MUTEX_LOCKED;
}

BYTE GetSecurityMutex()
{
  if (fSecurityMutex == MUTEX_LOCKED)
  {
    return FALSE;
  }
  fSecurityMutex = MUTEX_LOCKED;
  return TRUE;
}

void NonceHasBeenReceived()
{
  ZW_DEBUG_SECURITY_SEND_STR("NonceHasBeenReceived\n\r");
  /**The nonceIsReceived prevent the node from duplicate the tranmission of the msg encap**/
  /*Set when SendDataSecure return TRUE and cleared when it is done or failed.*/
  if (nonceIsReceived)
    return;
  if (IsSecurityMutexLocked())
  {
    PostEvent(EV_NONCE_REPORT_ARRIVES);
    if(!SendDataSecure(enNodeID, nrpBufData, nrDataLength, TRANSMIT_OPTION_EXPLORE, ZCB_SecureSessionDone))
    {
      ZCB_SecureSessionDone(TRANSPORT_SECURITY_BUSY, NULL);
    }
    else
    {
      nonceIsReceived = TRUE;
    }
  }
  else
  {
    ZW_DEBUG_SECURITY_SEND_STR("Unsolicited nonce received");
  }
}

ZCB_BYTE_PTR(ZCB_NonceReportCompletedLate)(BYTE bStatus, void *psTxResult)
{
  UNUSED(psTxResult);
  /* FIXME: Make sure we didnt start FLiRS timer at this point
   * ... or we could just stop it again */
  ZW_DEBUG_SECURITY_SEND_STR("Going to sleep here?\r\n");
  isNonceValid = 0;
  peerNodeIdWaitingForNonceReport = INVALID_NODE_ID;
  if (TRANSMIT_COMPLETE_OK == bStatus)
  {
    PostEvent(EV_NONCE_REPORT_TRANSMIT_OK);
  }
  else
  {
    PostEvent(EV_RETURN_TO_IDLE);
  }
}

void SecurityReturnToIdle()
{
  BYTE retVal;
  retVal = DeliverDelayedAppCmdHandler();
#ifdef ZW_DEBUG_SECURITY
  if (retVal
      && (ST_IDLE == GetSecurityFsmState())
      && (peerNodeIdWaitingForNonceReport != INVALID_NODE_ID))
  {
    ZW_DEBUG_SECURITY_SEND_STR("TxNonce late right after Delayed App Cmd\r\n");
  }
#endif
  if (ST_IDLE != GetSecurityFsmState())
  {
    /* Postpone delivery of waiting
     * Nonce Report until next IDLE
     * transition. */
    return;
  }
  /* since Security_FSM is still idle, we
   * can safely deliver the waiting Nonce Report */
  if (INVALID_NODE_ID != peerNodeIdWaitingForNonceReport)
  {
    /* We have postponed sending a Nonce Report.
     * Now is the time to resume. */
    noncePacket[0] = COMMAND_CLASS_SECURITY;
    /* Set nonce packet header */
    noncePacket[1] = SECURITY_NONCE_REPORT;
    /* Generate nonce */
    MakeIN(peerNodeIdWaitingForNonceReport, noncePacket + 2);
    isNonceValid = 1;
    ZW_DEBUG_SECURITY_SEND_STR("Tx nonce late: ");
    ZW_DEBUG_SECURITY_SEND_NUM(noncePacket[2]);
    ZW_DEBUG_SECURITY_SEND_NUM(noncePacket[3]);
    ZW_DEBUG_SECURITY_SEND_NUM(noncePacket[4]);
    ZW_DEBUG_SECURITY_SEND_NL();
    /* Send packet */
    if (!ZW_SendData(peerNodeIdWaitingForNonceReport, noncePacket, 10, txOptionsWaitingForNonceReport, ZCB_NonceReportCompletedLate))
    {
      ZCB_NonceReportCompletedLate(TRANSMIT_COMPLETE_FAIL, NULL);
      ZW_DEBUG_SECURITY_SEND_STR("\r\nNR Tx late fail\r\n");
    }
    else
    {
      PostEvent(EV_NONCE_REPORT_TRANSMITTING);
    }
    StartSecuritySendTimeOut(NONCE_TIMER);
  }
}
