#ifndef REMOTEIO_MIC_H__
#define REMOTEIO_MIC_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { s_ok, s_err } RioMicStat_t;

typedef RioMicStat_t (*fnAudioCallbackFn)(void *pUserData, float *pSampsBuf,
                                          int numChannels, int numFrames);

typedef struct {
  void *pUserData;
  fnAudioCallbackFn audCb;
  int fs;
} RioInstance_t;

// Starts the microphone, which will start triggering callbacks.
RioInstance_t *rio_start_mic(void *const pSelf, void *const pUserData,
                             fnAudioCallbackFn fnPtr, const int desiredFs);

void rio_stop_mic(RioInstance_t *pInst);

#ifdef __cplusplus
}
#endif

#endif
