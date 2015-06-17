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
#include "DaTunerApi.h"
#include <math.h>
#include <cstdlib>
#include <string.h>

extern "C" {

  typedef struct MyCounterStruct_tag {
    MIC_t mic;
    int mCallbackCount;
    DaTunerParameters prm;
    DaTunerAL al;
    float lastFreq;
    float lastSignal;
    RioInstance_t *pRio;
    bool initialized;
  } MyCounterStruct_t;

static RioMicStat_t myAudioCallback(void *pUserData, float *pSampsBuf,
                             int numChannels, int numFrames)
  {
    MyCounterStruct_t * pCounter = (MyCounterStruct_t *)pUserData;
    if (pCounter->initialized) {

      DaTunerAddSamplesFloat( pSampsBuf, numChannels * numFrames );
      int rval2 = -2;
      int rval1 = -1;
      while ((rval2 != rval1) && (rval1 != 0)) {
        rval2 = rval1;
        rval1 = DaTunerPoll();
      }

      pCounter->mCallbackCount++;
      if (pCounter->mCallbackCount & 0xff) {
        printf("%d: strength = %f: freq = %f\n", 
          pCounter->mCallbackCount, pCounter->lastSignal, pCounter->lastFreq );
      }
    }
    return s_ok;
  }

static void	datuner_cb( void *pUserData, const DaTunerEventData * const pEventData ) 
{
  MyCounterStruct_t * pCounter = (MyCounterStruct_t *)pUserData;
  switch (pEventData->event) {
  case E_EVT_THRESHOLD_CHANGE:
    //printf("signal = 0x%f\n", pEventData->eventData.thresholdChange.dBFSSignal);
    pCounter->lastSignal = pEventData->eventData.thresholdChange.dBFSSignal;
    break;
  case E_EVT_NOTE_DETECTED:
    //printf("freq = 0x%f\n", pEventData->eventData.noteDetected.freqHz);
    pCounter->lastFreq = pEventData->eventData.noteDetected.freqHz;
    break;
  default:
    break;
  }
}

static void datuner_trace( void *pUserData, const char *pTraceInfo ) {
  printf("%s\n", pTraceInfo);
}

MIC_t * MIC_Start( void * pSelf ) { 
  MyCounterStruct_t * pCounter = (MyCounterStruct_t *)malloc(sizeof(MyCounterStruct_t) );
  memset( pCounter, 0, sizeof( MyCounterStruct_t ) );
  DaTunerGetDefaults( &pCounter->prm );
  pCounter->al.OnDaTunerEventFnPtr = datuner_cb;
  pCounter->al.OnDbgTraceFnPtr = datuner_trace;
  pCounter->pRio = rio_start_mic( pSelf, pCounter, myAudioCallback );
  int fs = pCounter->pRio->fs;
  int decimation = fs / 9000;
  pCounter->prm.fFS = fs;
  pCounter->prm.decimationRatio = (DaTunerDecimation)decimation;
  pCounter->mic.pSelf = pSelf;
  printf("Initializing datuner with fs = %d, decimation = %d\n", fs, decimation );
  DaTunerInit( &pCounter->prm, &pCounter->al, pCounter );
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