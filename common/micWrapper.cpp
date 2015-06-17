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
  bool          pcmQInitialized;
} mw_t;

static mw_t mw;
static bool init = false;

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
          PcmQForceWrite(&mw.pcmQ, pSampsBuf, numSamples);
      }
      return s_ok;
  }


  /////////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////////
  int MicWStart() {
    if (!(init)) {
      memset(&mw, 0, sizeof(mw));
      init = true;
    }
    mw.pRio = rio_start_mic(NULL, &mw, mw_mic_callback);
    int bufSizeWords = (int)(1.0 * mw.pRio->fs);
    mw.pPcmBuf = (float *)malloc(bufSizeWords*sizeof(float));
    PcmQCreate(&mw.pcmQ, mw.pPcmBuf, bufSizeWords);
    mw.pcmQInitialized = true;      
    return mw.pRio->fs;
  }


  /////////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////////
  void MicWStop() {
    mw.pcmQInitialized = false;
    rio_stop_mic(mw.pRio);
    free(mw.pPcmBuf);
    mw.pPcmBuf = NULL;
    return;
  }


  /////////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////////
  int MicWGetReadReady() {
    int rval = 0;
    if (mw.pPcmBuf) {
      rval = PcmQGetReadReady(&mw.pcmQ);
    }
    return rval;
  }
  
  
  /////////////////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////////
  int MicWRead(float *pfBuf, int length) {
    int rval = 0;
    if (mw.pPcmBuf) {
      rval = PcmQRead(&mw.pcmQ, pfBuf, length);
    }
    return rval;
  }

}
