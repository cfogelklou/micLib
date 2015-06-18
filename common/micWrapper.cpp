//
//  micWrapper.cpp
//
//  Created by Chris Fogelklou
//
//

#include "PcmQ.h"
#include "remoteio_mic_c.h"
#include "../common/micwrapper_MicW.h"
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

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y));
#endif

typedef struct mw_tag {
  void          *pSelf;
  MWPcmQ_t        pcmQ;
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
          MWPcmQForceWrite(&mw.pcmQ, pSampsBuf, numSamples);
      }
      return s_ok;
}


/*
* Class:     micwrapper_MicW
* Method:    Start
* Signature: ()I
*/
jint JNICALL Java_micwrapper_MicW_Start
    (JNIEnv * pEnv, jclass c, jint fs)
{
    if (!(init)) {
        memset(&mw, 0, sizeof(mw));
        init = true;
    }
    mw.pRio = rio_start_mic(NULL, &mw, mw_mic_callback);
    int bufSizeWords = (int)(1.0 * mw.pRio->fs);
    mw.pPcmBuf = (float *)malloc(bufSizeWords*sizeof(float));
    MWPcmQCreate(&mw.pcmQ, mw.pPcmBuf, bufSizeWords);
    mw.pcmQInitialized = true;
    return mw.pRio->fs;
}


/*
* Class:     micwrapper_MicW
* Method:    Stop
* Signature: ()I
*/
jint JNICALL Java_micwrapper_MicW_Stop
    (JNIEnv * pEnv, jclass c) {
    mw.pcmQInitialized = false;
    rio_stop_mic(mw.pRio);
    free(mw.pPcmBuf);
    mw.pPcmBuf = NULL;
    return 0;
}


/*
* Class:     micwrapper_MicW
* Method:    GetReadReady
* Signature: ()I
*/
jint JNICALL Java_micwrapper_MicW_GetReadReady
    (JNIEnv * pEnv, jclass c) {
    int rval = 0;
    if (mw.pPcmBuf) {
        rval = MWPcmQGetReadReady(&mw.pcmQ);
    }
    return rval;
}


/*
* Class:     micwrapper_MicW
* Method:    Read
* Signature: ([FI)I
*/
jint JNICALL Java_micwrapper_MicW_Read
    (JNIEnv *pEnv, jclass c, jfloatArray jfBuf, jint length)
{
    jboolean jFalse = 0;
    float *pfBuf = pEnv->GetFloatArrayElements(jfBuf, &jFalse);
    jsize jlen = pEnv->GetArrayLength(jfBuf);
    length = MIN(length, jlen);
    int rval = 0;
    if (mw.pPcmBuf) {
        rval = MWPcmQRead(&mw.pcmQ, pfBuf, length);
    }

    pEnv->ReleaseFloatArrayElements(jfBuf, pfBuf, 0);
    return rval;
}

}
