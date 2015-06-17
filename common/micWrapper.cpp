//
//  micWrapper.c
//  Unity-iPhone
//
//  Created by Chris Fogelklou
//
//

#include "PcmQ.h"
#include "remoteio_mic_c.h"
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <pthread.h>

#ifndef ASSERT
#define ASSERT(x) if (!(x)) {printf("\n***Assertion Failed at %s(%d)***\n", __FILE__, __LINE__); exit(-1);}
#endif
#ifndef ASSERT_FN
#define ASSERT_FN(x) if (!(x)) {printf("\n***Assertion Failed at %s(%d)***\n", __FILE__, __LINE__); exit(-1);}
#endif


typedef struct mw_tag {
  void          *pSelf;
  PcmQ_t        pcmQ;
  float         *pPcmBuf;
  RioInstance_t *pRio;
  pthread_mutex_t   mutex;
  bool          pcmQInitialized;
} mw_t;

static mw_t mw;
static bool init = false;

#ifdef PTHREAD_RMUTEX_INITIALIZER
const pthread_mutex_t rMutexInit = PTHREAD_RMUTEX_INITIALIZER;
#else
#ifdef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
const pthread_mutex_t rMutexInit = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#else
const pthread_mutex_t rMutexInit = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
#endif
#endif

extern "C" {
  /////////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////////
  static RioMicStat_t mw_mic_callback(
    void *pUserData,
    float *pSampsBuf,
    int numChannels,
    int numFrames)
  {
    mw_t * pMw = (mw_t *)pUserData;
    ASSERT(pMw == &mw);
    ASSERT(numChannels == 1);
    if (mw.pcmQInitialized){
    const int numSamples = numChannels*numFrames;
    pthread_mutex_lock(&mw.mutex);
    PcmQForceWrite(&mw.pcmQ, pSampsBuf, numSamples);
    pthread_mutex_unlock(&mw.mutex);
    }
    return s_ok;
  }


  /////////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////////
  int _IPhoneMicStart() {
    if (!(init)) {
      memset(&mw, 0, sizeof(mw));
      memcpy(&mw.mutex, (void *)&rMutexInit, sizeof(pthread_mutex_t));
      init = true;
    }
    mw.pRio = rio_start_mic(NULL, &mw, mw_mic_callback);
    int bufSizeWords = (int)(1.0 * mw.pRio->fs);
    mw.pPcmBuf = (float *)malloc(bufSizeWords*sizeof(float));
    PcmQCreate(&mw.pcmQ, mw.pPcmBuf, bufSizeWords, PcmQ_NoMutexes);
    mw.pcmQInitialized = true;      
    return mw.pRio->fs;
  }


  /////////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////////
  void _IPhoneMicStop() {
    mw.pcmQInitialized = false;
    rio_stop_mic(mw.pRio);
    free(mw.pPcmBuf);
    mw.pPcmBuf = NULL;
    return;
  }


  /////////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////////
  int _IPhoneMicGetReadReady() {
    int rval = 0;
    if (mw.pPcmBuf) {
      pthread_mutex_lock(&mw.mutex);
      rval = PcmQGetReadReady(&mw.pcmQ);
      pthread_mutex_unlock(&mw.mutex);
    }
    return rval;
  }
  
  
  /////////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////////
  int _IPhoneMicRead(float *pfBuf, int length) {
    int rval = 0;
    if (mw.pPcmBuf) {
      pthread_mutex_lock(&mw.mutex);
      rval = PcmQRead(&mw.pcmQ, pfBuf, length);
      pthread_mutex_unlock(&mw.mutex);
    }
    return rval;
  }

}
