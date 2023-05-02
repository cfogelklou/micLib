#ifndef AUDIOCAPTURER_CPP_H__
#define AUDIOCAPTURER_CPP_H__ 1
// C-callable wrapper functions
#ifdef __cplusplus
extern "C" {
#endif

void *AudioCapturer_create(float sampleRate, void (*callback)(void *, float *, int), void *pUserData);
void AudioCapturer_startCapture(void *audioCapturer);
void AudioCapturer_stopCapture(void *audioCapturer);
void AudioCapturer_destroy(void *audioCapturer);

#ifdef __cplusplus
}
#endif

#endif
