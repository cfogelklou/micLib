#ifndef __PcmQ
#define __PcmQ

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

typedef struct _PcmQ_t
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

} PcmQ_t;

#ifdef __cplusplus
extern "C" {
#endif


  /** [Declaration] Initialize a queue */
  bool_t PcmQCreate( PcmQ_t * const pQ, pcm_t *pBuf, uint_t nBufSz );

  /** [Declaration] Destroy a queue */
  bool_t PcmQDestroy( PcmQ_t * const pQ );

  /** [Declaration] Write to a queue */
  uint_t PcmQWrite( PcmQ_t * const pQ, const pcm_t *pWrBuf, uint_t nLen );

  /** [Declaration] Generic queue read function */
  uint_t PcmQRead( PcmQ_t * const pQ, pcm_t *pRdBuf, uint_t nLen );

  /** [Declaration] Generic queue function for retrieving the amount of space in the buffer. */
  uint_t PcmQGetWriteReady( PcmQ_t * const pQ );

  /** [Declaration] Generic queue function for retrieving the amount of space in the buffer. */
  uint_t PcmQGetContiguousWriteReady( PcmQ_t * const pQ );

  /** [Declaration] Generic queue function for retrieving the number of bytes that can be read from the buffer. */
  uint_t PcmQGetReadReady( PcmQ_t * const pQ );

  /** [Declaration] Generic queue function for retrieving the number of bytes that can be read from the buffer. */
  uint_t PcmQGetContiguousReadReady( PcmQ_t * const pQ );

  /** [Declaration] Flushes the buffer. */
  void PcmQFlush( PcmQ_t * const pQ );

  /** [Declaration] Generic queue peek function */
  uint_t PcmQPeek( PcmQ_t * const pObj, pcm_t *pRdBuf, uint_t nLen );

  /** [Declaration] Commit an external write to the queue (simply increments the pointers and counters) */
  uint_t PcmQCommitWrite( PcmQ_t * const pQ, uint_t nLen );

  /** [Declaration] Commit an external read to the queue (simply increments the pointers and counters) */
  uint_t PcmQCommitRead( PcmQ_t * const pQ, uint_t nLen );

  /** [Declaration] Gets the write pointer */
  void * PcmQGetWritePtr( PcmQ_t * const pQ );

  /** [Declaration] Gets the read pointer */
  void * PcmQGetReadPtr( PcmQ_t * const pQ );

  /** [Declaration] Sets the read index based on a pointer into the internal buffer. */
  void PcmQSetRdIdxFromPointer( PcmQ_t * const pQ, void *pRdPtr );

  /** [Declaration] Writes a message to the queue.  Unsafe = not protected 
  internally by a mutex.
  Msg writes use additional internal headers so if a queue is to be used for 
  messages it must only be used for messages.
  */
  uint_t PcmQMsgWriteUnsafe( PcmQ_t * const pQ, pcm_t *pMsg, uint_t nLen, void **ppMsgBegin );

  /** [Declaration] Gets a pointer to the next message in the queue.
  Msg reads use additional internal headers so if a queue is to be used for 
  messages it must only be used for messages.
  */
  uint_t PcmQMsgGetNextPtrUnsafe( PcmQ_t * const pQ, void **ppMsgBegin );

  /** [Declaration] Commits read of the message pointer returned by PcmQMsgGetNextPtrUnsafe().
  Msg reads use additional internal headers so if a queue is to be used for 
  messages it must only be used for messages.
  */
  uint_t PcmQMsgCommitReadUnsafe( PcmQ_t * const pQ, void *pMsgBegin );

  /** [Declaration] Adjusts internal variables based on the new read pointer. */
  void PcmQMsgSetRdIdxFromPtrUnsafe( PcmQ_t * const pQ, void *pMsgBegin, uint_t msgLen );

  /** [Declaration] "Unreads" some data */
  uint_t PcmQUnread( PcmQ_t * const pQ, uint_t nLen );

  /** [Declaration] Forces the write of some data.  Advances the read pointer
  in the case that the buffer would have filled. */
  uint_t PcmQForceWrite( PcmQ_t * const pQ, const pcm_t * const pWrBuf, uint_t nLen );

  /** [Declaration] Peeks at data that resides somewhere inside the buffer */
  uint_t PcmQPeekRandom( PcmQ_t * const pQ, pcm_t *pRdBuf, uint_t bytesFromStart, uint_t nLen );

  /** [Declaration] Inserts data somewhere into the buffer */
  uint_t PcmQPokeRandom( PcmQ_t * const pQ, pcm_t *pWrBuf, uint_t bytesFromStart, uint_t nLen );

  /** [Declaration] Reads the last nLen words from the buffer */
  int_t PcmQDoReadFromEnd(PcmQ_t * const pQ, pcm_t *pRdBuf, int_t nLen);

      /** [Declaration] Reads the last nLen words from the buffer */
  int_t PcmQDoReadToDoubleFromEnd(PcmQ_t * const pQ, double *pRdBuf, int_t nLen);

  /** [Declaration] Not sure what this does, actually. */
  int_t PcmQDoReadToDoubleCustomCommit(
    PcmQ_t * const pQ, double *pRdBuf, uint_t nLen, int_t amountToCommit );

#ifdef __cplusplus
}
#endif

#endif
