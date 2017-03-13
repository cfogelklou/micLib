//
//  AVAudioSessionWrapper.h
//  remote_io
//
//  Created by Chris Fogelklou on 2017-03-12.
//  Copyright © 2017 Acorn Technology. All rights reserved.
//

#ifndef AVAudioSessionWrapper_h
#define AVAudioSessionWrapper_h

#include <AudioToolbox/AudioToolbox.h>

#ifdef __cplusplus
extern "C" {
#endif

// For backwards compatibility with legacy C++ code, implements C APIs for Get and Set (for microphone access)
// In a .mm file, since there are no C APIs for audio sessions anymore.

/*
 @function       AudioSessionGetProperty wrapper
 */
extern OSStatus _AudioSessionGetProperty(AudioSessionPropertyID inID,
                                         UInt32 *ioDataSize, void *outData);

/*
 @function       AudioSessionSetProperty wrapper
 */
extern OSStatus _AudioSessionSetProperty(AudioSessionPropertyID inID,
                                         UInt32 inDataSize, const void *inData);

#ifdef __cplusplus
}
#endif

#endif /* AVAudioSessionWrapper_h */
