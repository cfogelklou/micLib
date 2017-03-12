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

/*!
 @function       AudioSessionGetProperty
 @abstract       Get the value of a property.
 @discussion     This function can be called to get the value for a property of the AudioSession.
 Valid properties are listed in an enum above.
 @param          inID
 The AudioSessionPropertyID for which we want to get the value.
 @param          ioDataSize
 The size of the data payload.
 On entry it should contain the size of the memory pointed to by outData.
 On exit it will contain the actual size of the data.
 @param          outData
 The data for the property will be copied here.
 @return         kAudioSessionNoError if the operation was successful.  If the property is a
 write-only property or only available by way of property listeners,
 kAudioSessionUnsupportedPropertyError will be returned.  Other error codes
 listed under AudioSession Error Constants also apply to this function.
 */
extern OSStatus
_AudioSessionGetProperty(            AudioSessionPropertyID              inID,
                        UInt32                              *ioDataSize,
                        void                                *outData);

extern OSStatus
_AudioSessionSetProperty(            AudioSessionPropertyID              inID,
                        UInt32                              inDataSize,
                        const void                          *inData);

#endif /* AVAudioSessionWrapper_h */
