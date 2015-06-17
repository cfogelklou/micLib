#ifndef __MWPcmQ
#define __MWPcmQ

/******************************************************************************
  Copyright 2014 Chris Fogelklou, Applaud Apps (Applicaudia)

  This source code may under NO circumstances be distributed without the 
  express written permission of the author.

  @author: chris.fogelklou@gmail.com
*******************************************************************************/

#include "audiolib_types.h"
#if defined(__APPLE__)
#include <pthread.h>
#endif

#define pcm_t float

typedef struct _MWPcmQ_t
{
  pcm_t          *      pfBuf;
  uint_t                  nWrIdx;
  uint_t                  nRdIdx;
  uint_t                  nCount;
  uint_t                  nBufSz;
  uint32_t                nRdCount;
#if defined(__APPLE__)
  pthread_mutex_t   mutex;
#endif

} MWPcmQ_t;

#ifdef __cplusplus
extern "C" {
#endif


  /** [Declaration] Initialize a queue */
  bool_t MWPcmQCreate( MWPcmQ_t * const pQ, pcm_t *pBuf, uint_t nBufSz );

  /** [Declaration] Destroy a queue */
  bool_t MWPcmQDestroy( MWPcmQ_t * const pQ );

  /** [Declaration] Write to a queue */
  uint_t MWPcmQWrite( MWPcmQ_t * const pQ, const pcm_t *pWrBuf, uint_t nLen );

  /** [Declaration] Generic queue read function */
  uint_t MWPcmQRead( MWPcmQ_t * const pQ, pcm_t *pRdBuf, uint_t nLen );

  /** [Declaration] Generic queue function for retrieving the amount of space in the buffer. */
  uint_t MWPcmQGetWriteReady( MWPcmQ_t * const pQ );

  /** [Declaration] Generic queue function for retrieving the amount of space in the buffer. */
  uint_t MWPcmQGetContiguousWriteReady( MWPcmQ_t * const pQ );

  /** [Declaration] Generic queue function for retrieving the number of bytes that can be read from the buffer. */
  uint_t MWPcmQGetReadReady( MWPcmQ_t * const pQ );

  /** [Declaration] Generic queue function for retrieving the number of bytes that can be read from the buffer. */
  uint_t MWPcmQGetContiguousReadReady( MWPcmQ_t * const pQ );

  /** [Declaration] Flushes the buffer. */
  void MWPcmQFlush( MWPcmQ_t * const pQ );

  /** [Declaration] Generic queue peek function */
  uint_t MWPcmQPeek( MWPcmQ_t * const pObj, pcm_t *pRdBuf, uint_t nLen );

  /** [Declaration] Commit an external write to the queue (simply increments the pointers and counters) */
  uint_t MWPcmQCommitWrite( MWPcmQ_t * const pQ, uint_t nLen );

  /** [Declaration] Commit an external read to the queue (simply increments the pointers and counters) */
  uint_t MWPcmQCommitRead( MWPcmQ_t * const pQ, uint_t nLen );

  /** [Declaration] Gets the write pointer */
  void * MWPcmQGetWritePtr( MWPcmQ_t * const pQ );

  /** [Declaration] Gets the read pointer */
  void * MWPcmQGetReadPtr( MWPcmQ_t * const pQ );

  /** [Declaration] Sets the read index based on a pointer into the internal buffer. */
  void MWPcmQSetRdIdxFromPointer( MWPcmQ_t * const pQ, void *pRdPtr );

  /** [Declaration] Writes a message to the queue.  Unsafe = not protected 
  internally by a mutex.
  Msg writes use additional internal headers so if a queue is to be used for 
  messages it must only be used for messages.
  */
  uint_t MWPcmQMsgWriteUnsafe( MWPcmQ_t * const pQ, pcm_t *pMsg, uint_t nLen, void **ppMsgBegin );

  /** [Declaration] Gets a pointer to the next message in the queue.
  Msg reads use additional internal headers so if a queue is to be used for 
  messages it must only be used for messages.
  */
  uint_t MWPcmQMsgGetNextPtrUnsafe( MWPcmQ_t * const pQ, void **ppMsgBegin );

  /** [Declaration] Commits read of the message pointer returned by MWPcmQMsgGetNextPtrUnsafe().
  Msg reads use additional internal headers so if a queue is to be used for 
  messages it must only be used for messages.
  */
  uint_t MWPcmQMsgCommitReadUnsafe( MWPcmQ_t * const pQ, void *pMsgBegin );

  /** [Declaration] Adjusts internal variables based on the new read pointer. */
  void MWPcmQMsgSetRdIdxFromPtrUnsafe( MWPcmQ_t * const pQ, void *pMsgBegin, uint_t msgLen );

  /** [Declaration] "Unreads" some data */
  uint_t MWPcmQUnread( MWPcmQ_t * const pQ, uint_t nLen );

  /** [Declaration] Forces the write of some data.  Advances the read pointer
  in the case that the buffer would have filled. */
  uint_t MWPcmQForceWrite( MWPcmQ_t * const pQ, const pcm_t * const pWrBuf, uint_t nLen );

  /** [Declaration] Peeks at data that resides somewhere inside the buffer */
  uint_t MWPcmQPeekRandom( MWPcmQ_t * const pQ, pcm_t *pRdBuf, uint_t bytesFromStart, uint_t nLen );

  /** [Declaration] Inserts data somewhere into the buffer */
  uint_t MWPcmQPokeRandom( MWPcmQ_t * const pQ, pcm_t *pWrBuf, uint_t bytesFromStart, uint_t nLen );

  /** [Declaration] Reads the last nLen words from the buffer */
  int_t MWPcmQDoReadFromEnd(MWPcmQ_t * const pQ, pcm_t *pRdBuf, int_t nLen);

      /** [Declaration] Reads the last nLen words from the buffer */
  int_t MWPcmQDoReadToDoubleFromEnd(MWPcmQ_t * const pQ, double *pRdBuf, int_t nLen);

  /** [Declaration] Not sure what this does, actually. */
  int_t MWPcmQDoReadToDoubleCustomCommit(
    MWPcmQ_t * const pQ, double *pRdBuf, uint_t nLen, int_t amountToCommit );

#ifdef __cplusplus
}
#endif

#endif
