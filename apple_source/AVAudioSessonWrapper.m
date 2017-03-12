//
//  AVAudioSessonWrapper.m
//  remote_io
//
//  Created by Chris Fogelklou on 2017-03-12.
//  Copyright © 2017 Acorn Technology. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "AVAudioSessionWrapper.h"

void test(void)
{
    NSLog(@"This is a test function");
}

OSStatus
_AudioSessionGetProperty(            AudioSessionPropertyID              inID,
                         UInt32                              *ioDataSize,
                         void                                *outData){
    return noErr;
}


OSStatus
_AudioSessionSetProperty(            AudioSessionPropertyID              inID,
                        UInt32                              inDataSize,
                         const void                          *inData){
    return noErr;
}
