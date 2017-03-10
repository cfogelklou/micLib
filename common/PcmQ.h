#ifndef __MWPcmQ
#define __MWPcmQ

/******************************************************************************
  Copyright 2014 Chris Fogelklou, Applaud Apps (Applicaudia)

  This source code may under NO circumstances be distributed without the
  express written permission of the author.

  @author: chris.fogelklou@gmail.com
*******************************************************************************/

#if defined(__APPLE__)
#include <pthread.h>
#endif

#include <stdint.h>
#include <stdbool.h>

#define pcm_t float

typedef struct _MWPcmQ_t {
  pcm_t *pfBuf;
  unsigned int nWrIdx;
  unsigned int nRdIdx;
  unsigned int nCount;
  unsigned int nBufSz;
  uint32_t nRdCount;
#if defined(__APPLE__)
  pthread_mutex_t mutex;
#endif

} MWPcmQ_t;

#ifdef __cplusplus
extern "C" {
#endif

/** [Declaration] Initialize a queue */
bool MWPcmQCreate(MWPcmQ_t *const pQ, pcm_t *pBuf, unsigned int nBufSz);

/** [Declaration] Destroy a queue */
bool MWPcmQDestroy(MWPcmQ_t *const pQ);

/** [Declaration] Write to a queue */
unsigned int MWPcmQWrite(MWPcmQ_t *const pQ, const pcm_t *pWrBuf,
                         unsigned int nLen);

/** [Declaration] Generic queue read function */
unsigned int MWPcmQRead(MWPcmQ_t *const pQ, pcm_t *pRdBuf, unsigned int nLen);

/** [Declaration] Generic queue function for retrieving the amount of space in
 * the buffer. */
unsigned int MWPcmQGetWriteReady(MWPcmQ_t *const pQ);

/** [Declaration] Generic queue function for retrieving the amount of space in
 * the buffer. */
unsigned int MWPcmQGetContiguousWriteReady(MWPcmQ_t *const pQ);

/** [Declaration] Generic queue function for retrieving the number of bytes that
 * can be read from the buffer. */
unsigned int MWPcmQGetReadReady(MWPcmQ_t *const pQ);

/** [Declaration] Generic queue function for retrieving the number of bytes that
 * can be read from the buffer. */
unsigned int MWPcmQGetContiguousReadReady(MWPcmQ_t *const pQ);

/** [Declaration] Flushes the buffer. */
void MWPcmQFlush(MWPcmQ_t *const pQ);

/** [Declaration] Generic queue peek function */
unsigned int MWPcmQPeek(MWPcmQ_t *const pObj, pcm_t *pRdBuf, unsigned int nLen);

/** [Declaration] Commit an external write to the queue (simply increments the
 * pointers and counters) */
unsigned int MWPcmQCommitWrite(MWPcmQ_t *const pQ, unsigned int nLen);

/** [Declaration] Commit an external read to the queue (simply increments the
 * pointers and counters) */
unsigned int MWPcmQCommitRead(MWPcmQ_t *const pQ, unsigned int nLen);

/** [Declaration] Gets the write pointer */
void *MWPcmQGetWritePtr(MWPcmQ_t *const pQ);

/** [Declaration] Gets the read pointer */
void *MWPcmQGetReadPtr(MWPcmQ_t *const pQ);

/** [Declaration] Sets the read index based on a pointer into the internal
 * buffer. */
void MWPcmQSetRdIdxFromPointer(MWPcmQ_t *const pQ, void *pRdPtr);

/** [Declaration] Writes a message to the queue.  Unsafe = not protected
internally by a mutex.
Msg writes use additional internal headers so if a queue is to be used for
messages it must only be used for messages.
*/
unsigned int MWPcmQMsgWriteUnsafe(MWPcmQ_t *const pQ, pcm_t *pMsg,
                                  unsigned int nLen, void **ppMsgBegin);

/** [Declaration] Gets a pointer to the next message in the queue.
Msg reads use additional internal headers so if a queue is to be used for
messages it must only be used for messages.
*/
unsigned int MWPcmQMsgGetNextPtrUnsafe(MWPcmQ_t *const pQ, void **ppMsgBegin);

/** [Declaration] Commits read of the message pointer returned by
MWPcmQMsgGetNextPtrUnsafe().
Msg reads use additional internal headers so if a queue is to be used for
messages it must only be used for messages.
*/
unsigned int MWPcmQMsgCommitReadUnsafe(MWPcmQ_t *const pQ, void *pMsgBegin);

/** [Declaration] Adjusts internal variables based on the new read pointer. */
void MWPcmQMsgSetRdIdxFromPtrUnsafe(MWPcmQ_t *const pQ, void *pMsgBegin,
                                    unsigned int msgLen);

/** [Declaration] "Unreads" some data */
unsigned int MWPcmQUnread(MWPcmQ_t *const pQ, unsigned int nLen);

/** [Declaration] Forces the write of some data.  Advances the read pointer
in the case that the buffer would have filled. */
unsigned int MWPcmQForceWrite(MWPcmQ_t *const pQ, const pcm_t *const pWrBuf,
                              unsigned int nLen);

/** [Declaration] Peeks at data that resides somewhere inside the buffer */
unsigned int MWPcmQPeekRandom(MWPcmQ_t *const pQ, pcm_t *pRdBuf,
                              unsigned int bytesFromStart, unsigned int nLen);

/** [Declaration] Inserts data somewhere into the buffer */
unsigned int MWPcmQPokeRandom(MWPcmQ_t *const pQ, pcm_t *pWrBuf,
                              unsigned int bytesFromStart, unsigned int nLen);

/** [Declaration] Reads the last nLen words from the buffer */
int MWPcmQDoReadFromEnd(MWPcmQ_t *const pQ, pcm_t *pRdBuf, int nLen);

/** [Declaration] Reads the last nLen words from the buffer */
int MWPcmQDoReadToDoubleFromEnd(MWPcmQ_t *const pQ, double *pRdBuf,
                                  int nLen);

/** [Declaration] Not sure what this does, actually. */
int MWPcmQDoReadToDoubleCustomCommit(MWPcmQ_t *const pQ, double *pRdBuf,
                                       unsigned int nLen, int amountToCommit);

#ifdef __cplusplus
}
#endif

#endif
