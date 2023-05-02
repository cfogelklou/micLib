//
//  AppDelegate_cpp.cpp
//  remoteio_test
//
//  Created by Chris Fogelklou on 28/05/15.
//  Copyright (c) 2015 Applicaudia. All rights reserved.
//
#ifndef APPDELEGATE_CPP_H__
#define APPDELEGATE_CPP_H__
#include "AppDelegate_cpp.h"
#include "AudioCapturer_cpp.h"
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
  void *pAudioCapturer;
  float fs;
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
static void myAudioCallback(void *pUserData, float *pSampsBuf,
                                    int numSamps) {
  MyCounterStruct_t *pMicObj = (MyCounterStruct_t *)pUserData;
  if (pMicObj->initialized) {
    for (int i = 0; i < numSamps; i++) {
      pMicObj->rmsPower += (double)pSampsBuf[i] * (double)pSampsBuf[i];
    }
    pMicObj->rmsCount += numSamps;
    pMicObj->fsMeasureSampCount += numSamps;
    
    if (pMicObj->fsMeasureSampCount >= 44100 * 5){
      const auto now_ms = now();
      const auto diff_ms = now_ms - pMicObj->fsMeasureLastMs;
      double seconds = diff_ms / 1000.0;
      double fs = pMicObj->fsMeasureSampCount / seconds;
      printf("Measured fs = %f\n", fs);
      pMicObj->fsMeasureLastMs = now_ms;
      pMicObj->fsMeasureSampCount = 0;
    }
    
    pMicObj->mCallbackCount++;
    if (0 == (pMicObj->mCallbackCount & 0x1f)) {
      pMicObj->rmsPower = sqrt(pMicObj->rmsPower / pMicObj->rmsCount);
      printf("%d callbacks:pwr = %f\n", pMicObj->mCallbackCount,
             pMicObj->rmsPower);
        pMicObj->rmsPower = 0;
        pMicObj->rmsCount = 0;
      if ((pMicObj->pFft)){

        Fft &fft = *pMicObj->pFft;
        if (numSamps >= FFT_SIZE) {
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
          auto peakFreq = fft.IndexToFrequency(FFT_SIZE, maxIdx, pMicObj->fs);
          printf("peakFreq = %f\n", peakFreq);

        }
      }
    }
  }

}

// /////////////////////////////////////////////////////////////////////////////////////////////////
MIC_t *MIC_Start(void *pSelf) {
  MyCounterStruct_t *pMicObj = new MyCounterStruct_t;
  pMicObj->fs = 48000;
  pMicObj->pAudioCapturer = AudioCapturer_create(48000, myAudioCallback, pMicObj);
  pMicObj->mic.pSelf = pSelf;
  pMicObj->pFft = new Fft(FFT_SIZE);
  pMicObj->fsMeasureLastMs = now();
  pMicObj->initialized = true;
  AudioCapturer_startCapture(pMicObj->pAudioCapturer);

  return &pMicObj->mic;
}

// /////////////////////////////////////////////////////////////////////////////////////////////////
void MIC_Stop(MIC_t *pMic) {
  MyCounterStruct_t *pMicObj = (MyCounterStruct_t *)pMic;
  AudioCapturer_stopCapture(pMicObj->pAudioCapturer);
  AudioCapturer_destroy(pMicObj->pAudioCapturer);
  Fft *pFft = pMicObj->pFft;
  pMicObj->pFft = nullptr;
  delete pMicObj;
  pMicObj = nullptr;
  if (pFft){
    delete pFft;
    pFft = nullptr;
  }
}
}

#endif // ifndef APPDELEGATE_CPP_H__
