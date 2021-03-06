/******************************************************************************
  Copyright 2014 Chris Fogelklou, Applaud Apps (Applicaudia)

  This source code may under NO circumstances be distributed without the
  express written permission of the author.

  @author: chris.fogelklou@gmail.com
*******************************************************************************/
#include "PcmQ.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef LOG_ASSERT
#define LOG_ASSERT(x)                                                              \
  if (!(x)) {                                                                  \
    printf("\n***Assertion Failed at %s(%d)***\n", __FILE__, __LINE__);        \
    exit(-1);                                                                  \
  }
#endif
#ifndef LOG_ASSERT_FN
#define LOG_ASSERT_FN(x)                                                           \
  if (!(x)) {                                                                  \
    printf("\n***Assertion Failed at %s(%d)***\n", __FILE__, __LINE__);        \
    exit(-1);                                                                  \
  }
#endif

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y));
#endif

typedef int32_t msg_hdr_t;
#define MSG_ALIGN (sizeof(msg_hdr_t))
#define DOALIGN(_LEN) (((_LEN) + MSG_ALIGN - 1) & (unsigned int)(0 - MSG_ALIGN))

#if defined(__APPLE__)
#ifdef PTHREAD_RMUTEX_INITIALIZER
const pthread_mutex_t rMutexInit = PTHREAD_RMUTEX_INITIALIZER;
#else
#ifdef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
const pthread_mutex_t rMutexInit = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#else
const pthread_mutex_t rMutexInit = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
#endif
#endif

#define LOCKMUTEX(pQ) pthread_mutex_lock(&pQ->mutex)
#define UNLOCKMUTEX(pQ) pthread_mutex_unlock(&pQ->mutex)
#else
#define LOCKMUTEX(pQ)
#define UNLOCKMUTEX(pQ)
#endif

/* [Internal] Does an insertion at the current write pointer and count, but
does not update write pointer or count variables.
*/
static unsigned int MWPcmQUnprotectedInsert(MWPcmQ_t *const pQ, const pcm_t *pWrBuf,
                                      unsigned int nLen, unsigned int *pNewWrIdx);

/*
**=============================================================================
**  Abstract:
**    Public function to initialize the MWPcmQ_t structure.
**
**  Parameters:
**
**  Return values:
**
**=============================================================================
*/
bool MWPcmQCreate(MWPcmQ_t *const pQ, pcm_t *pBuf, unsigned int nBufSz) {

  LOG_ASSERT((NULL != pQ) && (NULL != pBuf));

  memset(pQ, 0, sizeof(MWPcmQ_t));

#if defined(__APPLE__)
  memcpy(&pQ->mutex, (void *)&rMutexInit, sizeof(pthread_mutex_t));
#endif

  pQ->pfBuf = pBuf;
  pQ->nBufSz = nBufSz;

  return true;
}

/*
**=============================================================================
**  Abstract:
**  Public function to deallocate the things in the
**   Q that were allocated.
**
**  Parameters:
**
**  Return values:
**
**=============================================================================
*/
bool MWPcmQDestroy(MWPcmQ_t *const pQ) {
  LOG_ASSERT(NULL != pQ);
  return true;
}

/*
**=============================================================================
**  Abstract:
**  Write function that lacks protection from multiple threads trying to write
**  at the same time.  Use this function if only one thread/task will write
**  to the queue at a time.
**  Parameters:
**
**  Return values:
**
**=============================================================================
*/
unsigned int MWPcmQWrite(MWPcmQ_t *const pQ, const pcm_t *pWrBuf, unsigned int nLen) {
  unsigned int WordsWritten = 0;

  LOG_ASSERT(NULL != pQ);

  if (nLen) {
    // Write nothing if there isn't room in the buffer.
    unsigned int WordsToWrite = 0;
    unsigned int nWrIdx = pQ->nWrIdx;
    const unsigned int nBufSz = pQ->nBufSz;
    pcm_t *const pBuf = pQ->pfBuf;

    WordsToWrite = (nLen <= (nBufSz - pQ->nCount)) ? nLen : 0;

    // We can definitely read WordsToWrite bytes.
    while (WordsToWrite > 0) {
      // Calculate how many contiguous bytes to the end of the buffer
      unsigned int Words = MIN(WordsToWrite, (nBufSz - nWrIdx));

      // Copy that many bytes.
      memcpy(&pBuf[nWrIdx], &pWrBuf[WordsWritten], Words * sizeof(pcm_t));

      // Circular buffering.
      nWrIdx += Words;
      if (nWrIdx >= nBufSz) {
        nWrIdx -= nBufSz;
      }

      // Increment the number of bytes written.
      WordsWritten += Words;
      WordsToWrite -= Words;
    }

    pQ->nWrIdx = nWrIdx;

    // Increment the count.  (protect with mutex)
    LOCKMUTEX(pQ);
    pQ->nCount = pQ->nCount + WordsWritten;
    UNLOCKMUTEX(pQ);
  }
  return WordsWritten;
}

/*
**=============================================================================
**=============================================================================
*/
static unsigned int MWPcmQUnprotectedInsert(MWPcmQ_t *const pQ, const pcm_t *pWrBuf,
                                      unsigned int nLen, unsigned int *pNewWrIdx) {
  unsigned int WordsWritten = 0;
  unsigned int nWrIdx = pQ->nWrIdx;

  if (nLen) {
    // Write nothing if there isn't room in the buffer.
    unsigned int WordsToWrite = nLen;
    const unsigned int nBufSz = pQ->nBufSz;
    pcm_t *const pBuf = pQ->pfBuf;

    while (WordsToWrite > 0) {
      unsigned int Words = MIN(WordsToWrite, (nBufSz - nWrIdx));

      memcpy(&pBuf[nWrIdx], &pWrBuf[WordsWritten], Words * sizeof(pcm_t));

      // Circular buffering.
      nWrIdx += Words;
      if (nWrIdx >= nBufSz) {
        nWrIdx -= nBufSz;
      }

      // Increment the number of bytes written.
      WordsWritten += Words;
      WordsToWrite -= Words;
    }
  }
  if (NULL != pNewWrIdx) {
    *pNewWrIdx = nWrIdx;
  }
  return WordsWritten;
}

/*
**=============================================================================
**=============================================================================
*/
unsigned int MWPcmQCommitWrite(MWPcmQ_t *const pQ, unsigned int nLen) {
  unsigned int WordsWritten = 0;

  LOG_ASSERT(NULL != pQ);

  if (nLen) {

    // Circular buffering.
    pQ->nWrIdx += nLen;
    if (pQ->nWrIdx >= pQ->nBufSz) {
      pQ->nWrIdx -= pQ->nBufSz;
    }

    // Increment the number of bytes written.
    WordsWritten += nLen;

    // Increment the count.  (protect with mutex)
    LOCKMUTEX(pQ);
    pQ->nCount = pQ->nCount + nLen;
    UNLOCKMUTEX(pQ);
  }
  return WordsWritten;
}

/*
**=============================================================================
**  Abstract:
**  Read function that lacks protection from multiple threads trying to read
**  at the same time.  Use this function if only one thread/task will read
**  from the queue at a time.
**  Parameters:
**
**  Return values:
**
**=============================================================================
*/
unsigned int MWPcmQRead(MWPcmQ_t *const pQ, pcm_t *pRdBuf, unsigned int nLen) {
  unsigned int WordsRead = 0;
  LOG_ASSERT(NULL != pQ);

  if (nLen) {
    // Calculate how many bytes can be read from the RdBuffer.
    unsigned int WordsToRead = 0;
    const unsigned int nBufSz = pQ->nBufSz;
    unsigned int nRdIdx = pQ->nRdIdx;
    const pcm_t *const pBuf = pQ->pfBuf;

    // No count MUTEX needed because count is native integer (single cycle write
    // or read)
    // and can only get larger if a process writes while we are reading.
    WordsToRead = MIN(pQ->nCount, nLen);

    // We can definitely read WordsToRead bytes.
    while (WordsToRead > 0) {
      // Calculate how many contiguous bytes to the end of the buffer
      unsigned int Words = MIN(WordsToRead, (nBufSz - nRdIdx));

      // Copy that many bytes.
      memcpy(&pRdBuf[WordsRead], &pBuf[nRdIdx], Words * sizeof(pcm_t));

      // Circular buffering.
      nRdIdx += Words;
      if (nRdIdx >= nBufSz) {
        nRdIdx -= nBufSz;
      }

      // Increment the number of bytes read.
      WordsRead += Words;
      WordsToRead -= Words;
    }

    pQ->nRdIdx = nRdIdx;

    // Decrement the count.
    LOCKMUTEX(pQ);
    pQ->nCount = pQ->nCount - WordsRead;
    UNLOCKMUTEX(pQ);
  }
  return WordsRead;
}

/*
**=============================================================================
**  Abstract:
**  Read function that lacks protection from multiple threads trying to read
**  at the same time.  Use this function if only one thread/task will read
**  from the queue at a time.
**  Parameters:
**
**  Return values:
**
**=============================================================================
*/
unsigned int MWPcmQCommitRead(MWPcmQ_t *const pQ, unsigned int nLen) {
  unsigned int WordsRead = 0;
  LOG_ASSERT(NULL != pQ);

  if (nLen) {

    // No count MUTEX needed because count is native integer (single cycle write
    // or read)
    // and can only get larger if a process writes while we are reading.
    nLen = MIN(pQ->nCount, nLen);

    pQ->nRdIdx += nLen;
    if (pQ->nRdIdx >= pQ->nBufSz) {
      pQ->nRdIdx -= pQ->nBufSz;
    }

    // Increment the number of bytes read.
    WordsRead += nLen;

    // Decrement the count.
    LOCKMUTEX(pQ);
    pQ->nCount = pQ->nCount - nLen;
    UNLOCKMUTEX(pQ);
  }
  return WordsRead;
}

/*
**=============================================================================
**  Abstract:
**
**  Parameters:
**
**  Return values:
**
**=============================================================================
*/
unsigned int MWPcmQGetWriteReady(MWPcmQ_t *const pQ) {
  unsigned int rval = 0;
  LOG_ASSERT(NULL != pQ);

  LOCKMUTEX(pQ);
  rval = (pQ->nBufSz - pQ->nCount);
  UNLOCKMUTEX(pQ);

  return rval;
}

/*
**=============================================================================
**  Abstract:
**
**  Parameters:
**
**  Return values:
**
**=============================================================================
*/
unsigned int MWPcmQGetContiguousWriteReady(MWPcmQ_t *const pQ) {

  unsigned int bytesReady = 0;
  LOG_ASSERT(NULL != pQ);

  LOCKMUTEX(pQ);
  bytesReady = (pQ->nBufSz - pQ->nCount);
  bytesReady = MIN(bytesReady, pQ->nBufSz - pQ->nWrIdx);
  UNLOCKMUTEX(pQ);
  return bytesReady;
}

/*
**=============================================================================
**  Abstract:
**
**  Parameters:
**
**  Return values:
**
**=============================================================================
*/
unsigned int MWPcmQGetReadReady(MWPcmQ_t *const pQ) {
  unsigned int bytesReady = 0;
  LOG_ASSERT(NULL != pQ);

  LOCKMUTEX(pQ);
  bytesReady = pQ->nCount;
  UNLOCKMUTEX(pQ);

  return bytesReady;
}

/*
**=============================================================================
**  Abstract:
**
**  Parameters:
**
**  Return values:
**
**=============================================================================
*/
unsigned int MWPcmQGetContiguousReadReady(MWPcmQ_t *const pQ) {

  unsigned int bytesReady = 0;
  LOG_ASSERT(NULL != pQ);

  LOCKMUTEX(pQ);
  bytesReady = MIN(pQ->nCount, pQ->nBufSz - pQ->nRdIdx);
  UNLOCKMUTEX(pQ);

  return bytesReady;
}

/*
**=============================================================================
**  Abstract:
**
**  Parameters:
**
**  Return values:
**
**=============================================================================
*/
void MWPcmQFlush(MWPcmQ_t *const pQ) {
  // Get the read mutex to only allow a single thread to read from the
  // queue at a time.

  LOG_ASSERT(NULL != pQ);

  // delete the single count mutex
  LOCKMUTEX(pQ);

  pQ->nCount = 0;
  pQ->nRdIdx = pQ->nWrIdx = 0;

  UNLOCKMUTEX(pQ);
}

/*
**=============================================================================
*  Abstract:
*
**=============================================================================
*/
unsigned int MWPcmQPeek(MWPcmQ_t *const pQ, pcm_t *pRdBuf, unsigned int nLen) {
  unsigned int WordsRead = 0;

  LOG_ASSERT(NULL != pQ);
  if (nLen) {

    unsigned int nRdIdx;
    unsigned int WordsToRead;

    nRdIdx = pQ->nRdIdx;

    // Calculate how many bytes can be read from the RdBuffer.
    WordsToRead = MIN(pQ->nCount, nLen);

    // We can definitely read WordsToRead bytes.
    while (WordsToRead > 0) {
      // Calculate how many contiguous bytes to the end of the buffer
      unsigned int Words = MIN(WordsToRead, (pQ->nBufSz - nRdIdx));

      // Copy that many bytes.
      memcpy(&pRdBuf[WordsRead], &pQ->pfBuf[nRdIdx], Words * sizeof(pcm_t));

      // Circular buffering.
      nRdIdx += Words;
      if (nRdIdx >= pQ->nBufSz) {
        nRdIdx -= pQ->nBufSz;
      }

      // Increment the number of bytes read.
      WordsRead += Words;
      WordsToRead -= Words;
    }
  }
  return WordsRead;
}

/*
**=============================================================================
*  Abstract:
*
**=============================================================================
*/
void *MWPcmQGetWritePtr(MWPcmQ_t *const pQ) {
  void *pRVal = 0;

  LOCKMUTEX(pQ);
  pRVal = (void *)&pQ->pfBuf[pQ->nWrIdx];
  UNLOCKMUTEX(pQ);

  return pRVal;
}

/*
**=============================================================================
*  Abstract:
*
**=============================================================================
*/
void *MWPcmQGetReadPtr(MWPcmQ_t *const pQ) {
  void *pRVal = 0;

  LOCKMUTEX(pQ);
  pRVal = (void *)&pQ->pfBuf[pQ->nRdIdx];
  UNLOCKMUTEX(pQ);

  return pRVal;
}

/*
**=============================================================================
*  Abstract:
*
**=============================================================================
*/
void MWPcmQSetRdIdxFromPointer(MWPcmQ_t *const pQ, void *pRdPtr) {
  LOCKMUTEX(pQ);
  {
    const pcm_t *const pRd8 = (const pcm_t *)pRdPtr;
    intptr_t newRdIdx = pRd8 - pQ->pfBuf; // lint !e946

    // Check for within range.
    if ((newRdIdx >= 0) && (newRdIdx <= (int)pQ->nBufSz)) // lint !e574 !e737
    {
      intptr_t newCount;

      // If last read advanced pointer to end of buffer, this is OK, just set to
      // beginning.
      if (newRdIdx == (int)pQ->nBufSz) // lint !e737
      {
        newRdIdx = 0;
      }

      // New count is amount write is ahead of read.
      newCount = (int)pQ->nWrIdx - newRdIdx;

      // Assume we are being called from consumer, so wr==rd results in zero
      // count
      if (newCount < 0) {
        //(Lint):Info 737: Loss of sign in promotion from int to unsigned int
        //(Lint):Info 713: Loss of precision (assignment) (unsigned int to int)
        newCount += pQ->nBufSz; // lint !e713 !e737
      }

      // Set read index and count.
      pQ->nRdIdx = (unsigned int)newRdIdx;
      pQ->nCount = (unsigned int)newCount;
    }
  }
  UNLOCKMUTEX(pQ);
}

/*
**=============================================================================
*  Abstract:
*
**=============================================================================
*/
unsigned int MWPcmQUnread(MWPcmQ_t *const pQ, unsigned int nLen) {
  unsigned int bytesUnread = 0;
  LOG_ASSERT(NULL != pQ);

  if (nLen) {
    // Calculate how many bytes can be read from the RdBuffer.
    unsigned int WordsToUnRead = 0;
    int nReadIdx = 0;

    // No count MUTEX needed because count is native integer (single cycle write
    // or read)
    // and can only get larger if a process writes while we are reading.
    WordsToUnRead = MIN((pQ->nBufSz - pQ->nCount), nLen);

    // We can definitely read WordsToRead bytes.
    nReadIdx = (int)pQ->nRdIdx - WordsToUnRead; // lint !e713 !e737
    if (nReadIdx < 0) {
      nReadIdx += pQ->nBufSz; // lint !e713 !e737
    }
    pQ->nRdIdx = (unsigned int)nReadIdx;

    // Decrement the count.
    LOCKMUTEX(pQ);
    pQ->nCount = pQ->nCount + WordsToUnRead;
    UNLOCKMUTEX(pQ);

    bytesUnread = WordsToUnRead;
  }
  return bytesUnread;
}

/*
**=============================================================================
*  Abstract:
*
**=============================================================================
*/
unsigned int MWPcmQForceWrite(MWPcmQ_t *const pQ, const pcm_t *const pWrBuf,
                        unsigned int nLen) {
  unsigned int words = 0;
  LOG_ASSERT(NULL != pQ);

  if (nLen) {
    int diff = 0;
    unsigned int writeableWords = 0;
    unsigned int newWrIdx = 0;

    LOCKMUTEX(pQ);

    // Calculate the number of bytes that can be written
    writeableWords = pQ->nBufSz - pQ->nCount;
    diff = nLen - writeableWords; // lint !e713 !e737

    // If more bytes should be written than there is space for,
    // force the read pointer forward
    if (diff > 0) {
      pQ->nRdIdx += diff; // lint !e713 !e737
      if (pQ->nRdIdx >= pQ->nBufSz) {
        pQ->nRdIdx -= pQ->nBufSz;
      }

      pQ->nCount -= diff; // lint !e713 !e737
    } else {
      diff = 0;
    }

    // Insert the data in the buffer.
    LOG_ASSERT_FN(nLen == MWPcmQUnprotectedInsert(pQ, pWrBuf, nLen, &newWrIdx));
    pQ->nWrIdx = newWrIdx;

    pQ->nCount += nLen;

    words = nLen;

    UNLOCKMUTEX(pQ);
  }
  return words;
}

/*
**=============================================================================
*  Abstract:
*
**=============================================================================
*/
unsigned int MWPcmQPeekRandom(MWPcmQ_t *const pQ, pcm_t *pRdBuf,
                        unsigned int bytesFromRdIdx, unsigned int nLen) {
  unsigned int WordsRead = 0;

  LOG_ASSERT(NULL != pQ);
  if (nLen) {

    unsigned int nRdIdx;
    unsigned int WordsToRead;
    int nCount;

    nRdIdx = pQ->nRdIdx + bytesFromRdIdx;
    if (nRdIdx >= pQ->nBufSz) {
      nRdIdx -= pQ->nBufSz;
    }
    nCount = (pQ->nCount - bytesFromRdIdx); // lint !e713 !e737
    nCount = (nCount < 0) ? 0 : nCount;

    // Calculate how many bytes can be read from the RdBuffer.
    WordsToRead = MIN(((unsigned int)nCount), nLen);

    // We can definitely read WordsToRead bytes.
    while (WordsToRead > 0) {
      // Calculate how many contiguous bytes to the end of the buffer
      unsigned int Words = MIN(WordsToRead, (pQ->nBufSz - nRdIdx));

      // Copy that many bytes.
      memcpy(&pRdBuf[WordsRead], &pQ->pfBuf[nRdIdx], Words * sizeof(pcm_t));

      // Circular buffering.
      nRdIdx += Words;
      if (nRdIdx >= pQ->nBufSz) {
        nRdIdx -= pQ->nBufSz;
      }

      // Increment the number of bytes read.
      WordsRead += Words;
      WordsToRead -= Words;
    }
  }
  return WordsRead;
}

/** [Declaration] Inserts data somewhere into the buffer */
unsigned int MWPcmQPokeRandom(MWPcmQ_t *const pQ, pcm_t *pWrBuf,
                        unsigned int bytesFromStart, unsigned int nLen) {
  unsigned int WordsWritten = 0;

  LOG_ASSERT(NULL != pQ);
  if (nLen) {

    unsigned int nWrIdx;
    unsigned int WordsToWrite;
    int nCount;

    nWrIdx = pQ->nWrIdx + bytesFromStart;
    if (nWrIdx >= pQ->nBufSz) {
      nWrIdx -= pQ->nBufSz;
    }

    // Can only write if it will fit within nCount
    nCount = pQ->nCount - bytesFromStart; // lint !e713 !e737
    nCount = (nCount < 0) ? 0 : nCount;

    // Calculate how many bytes can be written to the WrBuffer.
    WordsToWrite = MIN(((unsigned int)nCount), nLen);

    // We can definitely read WordsToRead bytes.
    while (WordsToWrite > 0) {
      // Calculate how many contiguous bytes to the end of the buffer
      unsigned int Words = MIN(WordsToWrite, (pQ->nBufSz - nWrIdx));

      // Copy that many bytes.
      memcpy(&pQ->pfBuf[nWrIdx], &pWrBuf[WordsWritten], Words * sizeof(pcm_t));

      // Circular buffering.
      nWrIdx += Words;
      if (nWrIdx >= pQ->nBufSz) {
        nWrIdx -= pQ->nBufSz;
      }

      // Increment the number of bytes read.
      WordsWritten += Words;
      WordsToWrite -= Words;
    }
  }
  return WordsWritten;
}

/** [Declaration] Reads the last nLen words from the buffer */
int MWPcmQDoReadFromEnd(MWPcmQ_t *const pQ, pcm_t *pRdBuf, int nLen) {
  int shortsRead = 0;

  if (nLen > 0) {
    // Calculate how many shorts can be read from the RdBuffer.
    int shortsToRead = 0;
    int nRdIdx = pQ->nWrIdx - nLen;
    if (nRdIdx < 0) {
      nRdIdx += pQ->nBufSz;
    }

    shortsToRead = nLen;

    // We can definitely read ShortsToRead shorts.
    while (shortsToRead > 0) {
      // Calculate how many contiguous shorts to the end of the buffer
      int Shorts = MIN(shortsToRead, (int)((pQ->nBufSz - nRdIdx)));

      // Copy that many shorts.
      memcpy(&pRdBuf[shortsRead], &pQ->pfBuf[nRdIdx], Shorts * sizeof(pcm_t));

      // Circular buffering.
      nRdIdx += Shorts;
      if (nRdIdx >= (int)pQ->nBufSz) {
        nRdIdx -= pQ->nBufSz;
      }

      // Increment the number of shorts read.
      shortsRead += Shorts;
      shortsToRead -= Shorts;
    }
  }
  return shortsRead;
}

int MWPcmQDoReadToDoubleFromEnd(MWPcmQ_t *const pQ, double *pRdBuf,
                                  int nLen) {
  int shortsRead = 0;

  if (nLen > 0) {
    // Calculate how many shorts can be read from the RdBuffer.
    int shortsToRead = 0;
    int nRdIdx = pQ->nWrIdx - nLen;
    if (nRdIdx < 0) {
      nRdIdx += pQ->nBufSz;
    }

    shortsToRead = nLen;

    // We can definitely read ShortsToRead shorts.
    while (shortsToRead > 0) {
      int i;

      // Calculate how many contiguous shorts to the end of the buffer
      int Shorts = MIN(shortsToRead, (int)((pQ->nBufSz - nRdIdx)));

      // Copy that many shorts.
      // HelperMath.aMemCpyToDbl( pRdBuf, shortsRead, pQ->pBuf, nRdIdx, Shorts
      // );
      for (i = 0; i < Shorts; i++) {
        pRdBuf[shortsRead + i] = (double)pQ->pfBuf[i + nRdIdx];
      }

      // Circular buffering.
      nRdIdx += Shorts;
      if (nRdIdx >= (int)pQ->nBufSz) {
        nRdIdx -= pQ->nBufSz;
      }

      // Increment the number of shorts read.
      shortsRead += Shorts;
      shortsToRead -= Shorts;
    }
  }
  return shortsRead;
}

int MWPcmQDoReadToDoubleCustomCommit(MWPcmQ_t *const pQ, double *pRdBuf,
                                       unsigned int nLen, int amountToCommit) {
  unsigned int nToRead = 0;
  // Calculate how many shorts can be read from the RdBuffer.
  unsigned int nRead = 0;

  if (pQ->nCount >= nLen) {

    unsigned int rdIdx = pQ->nRdIdx;
    // No count MUTEX needed because count is native integer (single cycle write
    // or read)
    // and can only get larger if a process writes while we are reading.
    nToRead = MIN(pQ->nCount, nLen);

    // We can definitely read ShortsToRead shorts.
    while (nToRead > 0) {
      unsigned int i;
      // Calculate how many contiguous shorts to the end of the buffer
      const unsigned int nShorts = MIN(nToRead, (pQ->nBufSz - rdIdx));

      // Copy that many shorts.
      // HelperMath.aMemCpyToDbl( pRdBuf, nRead, pQ->pBuf, rdIdx, nShorts );
      for (i = 0; i < nShorts; i++) {
        pRdBuf[nRead + i] = (double)pQ->pfBuf[i + rdIdx];
      }

      // Circular buffering.
      rdIdx += nShorts;
      if (rdIdx >= pQ->nBufSz) {
        rdIdx -= pQ->nBufSz;
      }

      // Increment the number of shorts read.
      nRead += nShorts;
      nToRead -= nShorts;
    }
    pQ->nRdIdx += amountToCommit;
    if (pQ->nRdIdx >= pQ->nBufSz) {
      pQ->nRdIdx -= pQ->nBufSz;
    }
    pQ->nRdCount += amountToCommit;

    LOCKMUTEX(pQ);
    pQ->nCount -= amountToCommit;
    UNLOCKMUTEX(pQ);
  }
  return nRead;
}
