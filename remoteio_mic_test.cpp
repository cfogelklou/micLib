
#include <cstdio>
#include "remoteio_mic_c.h"
#include <math.h>
using namespace std;


typedef struct MyCounterStruct_tag {
  int mCallbackCount;
  float avgPower;
} MyCounterStruct_t;

extern "C" {
  static RioMicStat_t micCallback( void *pUserData, float *pSampsBuf, int numChannels, int numFrames ) 
  {
    MyCounterStruct_t * pCounter = (MyCounterStruct_t *)pUserData;
    pCounter->mCallbackCount++;
    int numSamps = numChannels * numFrames;
    float rmsPwr = 0;
    for (int i = 0; i < numSamps; i++) {
      rmsPwr += pSampsBuf[i] * pSampsBuf[i];
    }
    rmsPwr = sqrtf(rmsPwr/numSamps);
    pCounter->avgPower = rmsPwr;
    return s_ok;
  }
}


int main (int argc, const char * argv[]) { 
  MyCounterStruct_t myCounter = {0};
  int fs = 0;
  void *pInst = rio_start_mic(NULL, &myCounter, micCallback);

  // And wait 
  printf("Capturing, press <return> to stop:\n"); 

  while(1) {
    getchar();
    printf("Sample count = %d\n", myCounter.mCallbackCount );
    printf("Power = %f\n", myCounter.avgPower );
  } 
  // Cleanup 

cleanup:
  //AUGraphStop (player.graph); 
  //AUGraphUninitialize (player.graph); 
  //AUGraphClose(player.graph);

  return 0;
}


extern "C" {
void DaTunerDbgTrace( const char * const pTraceInfo ) {
  printf(pTraceInfo);
}
}
