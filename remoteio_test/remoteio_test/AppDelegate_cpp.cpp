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
#include "fft.hpp"
#include <cstdlib>
#include <math.h>
#include <string.h>
#include <chrono>

extern "C" {

typedef struct MyCounterStruct_tag {
  MIC_t mic;
  int mCallbackCount;
  float rmsPower;
  int rmsCount;
  RioInstance_t *pRio;
  Fft *pFft;
  unsigned long fsMeasureLastMs;
  unsigned long fsMeasureSampCount;
  bool initialized;
} MyCounterStruct_t;

#define FFT_SIZE 1024
  
// /////////////////////////////////////////////////////////////////////////////
static unsigned long now(){
  using namespace std::chrono;
  auto now = system_clock::now();
  auto now_ms = time_point_cast<milliseconds>(now);
  auto value = now_ms.time_since_epoch();
  unsigned long duration = value.count();
  return duration;
}
  
// /////////////////////////////////////////////////////////////////////////////
static RioMicStat_t myAudioCallback(void *pUserData, float *pSampsBuf,
                                    int numChannels, int numFrames) {
  MyCounterStruct_t *pCounter = (MyCounterStruct_t *)pUserData;
  if (pCounter->initialized) {
    int numSamps = numChannels * numFrames;
    for (int i = 0; i < numSamps; i++) {
      pCounter->rmsPower += (double)pSampsBuf[i] * (double)pSampsBuf[i];
    }
    pCounter->rmsCount += numSamps;
    pCounter->fsMeasureSampCount += numFrames;
    
    if (pCounter->fsMeasureSampCount >= 44100 * 5){
      const auto now_ms = now();
      const auto diff_ms = now_ms - pCounter->fsMeasureLastMs;
      double seconds = diff_ms / 1000.0;
      double fs = pCounter->fsMeasureSampCount / seconds;
      printf("Measured fs = %f\n", fs);
      pCounter->fsMeasureLastMs = now_ms;
      pCounter->fsMeasureSampCount = 0;
    }
    
    pCounter->mCallbackCount++;
    if (0 == (pCounter->mCallbackCount & 0x1f)) {
      pCounter->rmsPower = sqrt(pCounter->rmsPower / pCounter->rmsCount);
      printf("%d callbacks:pwr = %f\n", pCounter->mCallbackCount,
             pCounter->rmsPower);
        pCounter->rmsPower = 0;
        pCounter->rmsCount = 0;
      if ((pCounter->pFft)){

        Fft &fft = *pCounter->pFft;
        if ((numChannels == 1) && (numFrames >= FFT_SIZE)) {
          static double mag[FFT_SIZE];
          static double phz[FFT_SIZE];
          static double real[FFT_SIZE];
          for (int i = 0; i < FFT_SIZE; i++){
            real[i] = pSampsBuf[i];
          }
          
          fft.DoFFT(real, nullptr, mag, phz, FFT_SIZE);
          fft.GetMagnitude(mag, phz, mag, FFT_SIZE/2);
          int maxIdx = 4;
          auto maxMag = mag[maxIdx];
          for (int i = 5; i < FFT_SIZE/2; i++){
            auto pk = mag[i];
            if (pk > maxMag){
              maxIdx = i;
              maxMag = pk;
            }
          }
          auto peakFreq = fft.IndexToFrequency(FFT_SIZE, maxIdx, pCounter->pRio->fs);
          printf("peakFreq = %f\n", peakFreq);

        }
      }
    }
  }
  return s_ok;
}

// /////////////////////////////////////////////////////////////////////////////////////////////////
MIC_t *MIC_Start(void *pSelf) {
  MyCounterStruct_t *pCounter =
      (MyCounterStruct_t *)malloc(sizeof(MyCounterStruct_t));
  memset(pCounter, 0, sizeof(MyCounterStruct_t));
  pCounter->pRio = rio_start_mic(pSelf, pCounter, myAudioCallback, 48000);
  // int fs = pCounter->pRio->fs;
  pCounter->mic.pSelf = pSelf;
  pCounter->pFft = new Fft(FFT_SIZE);
  pCounter->fsMeasureLastMs = now();

  pCounter->initialized = true;

  return &pCounter->mic;
}

// /////////////////////////////////////////////////////////////////////////////////////////////////
void MIC_Stop(MIC_t *pMic) {
  MyCounterStruct_t *pCounter = (MyCounterStruct_t *)pMic;
  rio_stop_mic(pCounter->pRio);
  Fft *pFft = pCounter->pFft;
  pCounter->pFft = nullptr;
  free(pCounter);
  pCounter = NULL;
  if (pFft){
    delete pFft;
  }
}
}

#endif // ifndef APPDELEGATE_CPP_H__
