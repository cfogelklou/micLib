//
//  AudioCapturer_cpp.h
//  MicrophoneAudioCapturer
//
//  The AudioCapturer class is responsible for setting up and managing an AVAudioEngine
//  instance to capture audio from the microphone, convert it to a mono float stream,
//  and pass it to the user-provided callback function.
//
//  The AudioCapturer also provides a set of C-callable wrapper functions that allow
//  the class to be easily integrated into C and C++ projects.
//
#ifndef AUDIOCAPTURER_CPP_H__
#define AUDIOCAPTURER_CPP_H__ 1
// C-callable wrapper functions
#ifdef __cplusplus
extern "C" {
#endif

void *AudioCapturer_create(float sampleRate, void (*callback)(void *, float *, int), void *pUserData);
float AudioCapturer_getActualSampleRate(void *audioCapturer);
void AudioCapturer_startCapture(void *audioCapturer);
void AudioCapturer_stopCapture(void *audioCapturer);
void AudioCapturer_destroy(void *audioCapturer);

#ifdef __cplusplus
}
#endif

#endif
