//
//  AppDelegate_cpp.cpp
//  remoteio_test
//
//  Created by Chris Fogelklou on 28/05/15.
//  Copyright (c) 2015 Acorn Technology. All rights reserved.
//
#ifndef APPDELEGATE_CPP_H__
#define APPDELEGATE_CPP_H__
#include "AppDelegate_cpp.h"
#include "remoteio_mic_c.h"
#include <math.h>
#include <cstdlib>
#include <string.h>

extern "C" {

  typedef struct MyCounterStruct_tag {
    MIC_t mic;
    int mCallbackCount;
    RioInstance_t *pRio;
    bool initialized;
  } MyCounterStruct_t;

static RioMicStat_t myAudioCallback(void *pUserData, float *pSampsBuf,
                             int numChannels, int numFrames)
  {
    MyCounterStruct_t * pCounter = (MyCounterStruct_t *)pUserData;
    if (pCounter->initialized) {

      pCounter->mCallbackCount++;
      if (pCounter->mCallbackCount & 0xff) {
        printf("%d: callbacks\n", pCounter->mCallbackCount);
      }
    }
    return s_ok;
  }



MIC_t * MIC_Start( void * pSelf ) { 
  MyCounterStruct_t * pCounter = (MyCounterStruct_t *)malloc(sizeof(MyCounterStruct_t) );
  memset( pCounter, 0, sizeof( MyCounterStruct_t ) );
  pCounter->pRio = rio_start_mic( pSelf, pCounter, myAudioCallback, 48000 );
  //int fs = pCounter->pRio->fs;
  pCounter->mic.pSelf = pSelf;
  pCounter->initialized = true;
  
  return &pCounter->mic; 
}
    
void MIC_Stop( MIC_t * pMic ) {
  MyCounterStruct_t *pCounter = (MyCounterStruct_t *)pMic;
  rio_stop_mic( pCounter->pRio );
  free( pCounter );
  pCounter = NULL;
}
}


#endif // ifndef APPDELEGATE_CPP_H__