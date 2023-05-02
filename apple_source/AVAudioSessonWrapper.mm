//
//  AVAudioSessonWrapper.m
//  remote_io
//
//  Created by Chris Fogelklou on 2017-03-12.
//  Copyright © 2017 Applicaudia. All rights reserved.
// Apple has disabled C/CPP access to AVAudioSession (unfortunately.)
// This file makes bringing support back easy... All we need is the shared
// session.
//

#import "AVAudioSessionWrapper.h"
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <Foundation/Foundation.h>
#import <assert.h>

class SessionWrapper {
private:
  AVAudioSession *mpSharedInstance;
  bool mGotRecordPermission;
  SessionWrapper() { mpSharedInstance = [AVAudioSession sharedInstance]; }

public:
  static SessionWrapper &inst() {
    static SessionWrapper mInst;
    return mInst;
  }

  AVAudioSession *sess() { return mpSharedInstance; }

  void gotPermission(BOOL granted) {
    mGotRecordPermission = (granted == YES) ? true : false;
  }
};

extern "C" {

/*
 SESSION PROPERTY ID
 kAudioSessionProperty_PreferredHardwareSampleRate           = 'hwsr',   //
 Float64          (get/set)
 kAudioSessionProperty_PreferredHardwareIOBufferDuration     = 'iobd',   //
 Float32          (get/set)
 kAudioSessionProperty_AudioCategory                         = 'acat',   //
 UInt32           (get/set)
 kAudioSessionProperty_AudioRouteChange                      = 'roch',   //
 CFDictionaryRef  (property listener)
 kAudioSessionProperty_CurrentHardwareSampleRate             = 'chsr',   //
 Float64          (get only)
 kAudioSessionProperty_CurrentHardwareInputNumberChannels    = 'chic',   //
 UInt32           (get only/property listener)
 kAudioSessionProperty_CurrentHardwareOutputNumberChannels   = 'choc',   //
 UInt32           (get only/property listener)
 kAudioSessionProperty_CurrentHardwareOutputVolume           = 'chov',   //
 Float32          (get only/property listener)
 kAudioSessionProperty_CurrentHardwareInputLatency           = 'cilt',   //
 Float32          (get only)
 kAudioSessionProperty_CurrentHardwareOutputLatency          = 'colt',   //
 Float32          (get only)
 kAudioSessionProperty_CurrentHardwareIOBufferDuration       = 'chbd',   //
 Float32          (get only)
 kAudioSessionProperty_OtherAudioIsPlaying                   = 'othr',   //
 UInt32           (get only)
 kAudioSessionProperty_OverrideAudioRoute                    = 'ovrd',   //
 UInt32           (set only)
 kAudioSessionProperty_AudioInputAvailable                   = 'aiav',   //
 UInt32           (get only/property listener)
 kAudioSessionProperty_ServerDied                            = 'died',   //
 UInt32           (property listener)
 kAudioSessionProperty_OtherMixableAudioShouldDuck           = 'duck',   //
 UInt32           (get/set)
 kAudioSessionProperty_OverrideCategoryMixWithOthers         = 'cmix',   //
 UInt32           (get, some set)
 kAudioSessionProperty_OverrideCategoryDefaultToSpeaker      = 'cspk',   //
 UInt32           (get, some set)
 kAudioSessionProperty_OverrideCategoryEnableBluetoothInput  = 'cblu',   //
 UInt32           (get, some set)
 kAudioSessionProperty_InterruptionType                      = 'type',   //
 UInt32           (get only)
 kAudioSessionProperty_Mode                                  = 'mode',   //
 UInt32           (get/set)
 kAudioSessionProperty_InputSources                          = 'srcs',   //
 CFArrayRef       (get only/property listener)
 kAudioSessionProperty_OutputDestinations                    = 'dsts',   //
 CFArrayRef       (get only/property listener)
 kAudioSessionProperty_InputSource                           = 'isrc',   //
 CFNumberRef      (get/set)
 kAudioSessionProperty_OutputDestination                     = 'odst',   //
 CFNumberRef      (get/set)
 kAudioSessionProperty_InputGainAvailable                    = 'igav',   //
 UInt32           (get only/property listener)
 kAudioSessionProperty_InputGainScalar                       = 'igsc',   //
 Float32          (get/set/property listener)
 kAudioSessionProperty_AudioRouteDescription                 = 'crar'    //
 CFDictionaryRef  (get only)


 CATEGORIES


 kAudioSessionCategory_AmbientSound               = 'ambi',
 kAudioSessionCategory_SoloAmbientSound           = 'solo',
 kAudioSessionCategory_MediaPlayback              = 'medi',
 kAudioSessionCategory_RecordAudio                = 'reca',
 kAudioSessionCategory_PlayAndRecord              = 'plar',
 kAudioSessionCategory_AudioProcessing            = 'proc'
 */
typedef struct {
  UInt32 avEnum;
  NSString *avStr;
} AVSW_ConvertLm;

static const AVSW_ConvertLm AVSW_ConvertLmArr[] = {
    {kAudioSessionCategory_AmbientSound, AVAudioSessionCategoryAmbient},
    {kAudioSessionCategory_SoloAmbientSound, AVAudioSessionCategorySoloAmbient},
    {kAudioSessionCategory_MediaPlayback, AVAudioSessionCategoryPlayback},
    {kAudioSessionCategory_RecordAudio, AVAudioSessionCategoryRecord},
    {kAudioSessionCategory_PlayAndRecord, AVAudioSessionCategoryPlayAndRecord},
    {kAudioSessionCategory_AudioProcessing,
     AVAudioSessionCategoryAudioProcessing},
    {kAudioSessionCategory_AmbientSound, AVAudioSessionCategoryMultiRoute},
};

static const int AVSW_ConvertLmArrSz =
    sizeof(AVSW_ConvertLmArr) / sizeof(AVSW_ConvertLmArr[0]);

static UInt32 avsw_GetkAudioSessionCategory(NSString *p) {
  UInt32 rval = kAudioSessionCategory_AmbientSound;
  int rvalIdx = -1;
  int i = 0;
  while ((rvalIdx < 0) && (i < AVSW_ConvertLmArrSz)) {
    if ([p isEqualToString:AVSW_ConvertLmArr[i].avStr]) {
      rvalIdx = i;
      rval = AVSW_ConvertLmArr[i].avEnum;
    }
    i++;
  }
  return rval;
}

static NSString *avsw_GetAVAudioSessionMode(UInt32 avEnum) {
  NSString *rval = AVAudioSessionCategoryAmbient;
  int rvalIdx = -1;
  int i = 0;
  while ((rvalIdx < 0) && (i < AVSW_ConvertLmArrSz)) {
    if (avEnum == AVSW_ConvertLmArr[i].avEnum) {
      rvalIdx = i;
      rval = AVSW_ConvertLmArr[i].avStr;
    }
    i++;
  }
  return rval;
}

OSStatus _AudioSessionGetProperty(AudioSessionPropertyID inID,
                                  UInt32 *ioDataSize, void *outData) {
  assert(ioDataSize);
  assert(outData);
  switch (inID) {
  case kAudioSessionProperty_AudioCategory: {
    NSLog(@"kAudioSessionProperty_AudioCategory");
    // SessionWrapper.inst().sess.
    NSString *p = [SessionWrapper::inst().sess() category];
    *(UInt32 *)outData = avsw_GetkAudioSessionCategory(p);
  } break;
  case kAudioSessionProperty_AudioInputAvailable: {
    UInt32 isInputAvailable = [SessionWrapper::inst().sess() isInputAvailable];
    *(UInt32 *)outData = isInputAvailable;
    if (isInputAvailable) {
      [SessionWrapper::inst().sess() requestRecordPermission:^(BOOL granted) {
        SessionWrapper::inst().gotPermission(granted);
      }];
    }
  } break;
  case kAudioSessionProperty_CurrentHardwareSampleRate:
    assert(sizeof(Float64) == *ioDataSize);
    *(Float64 *)outData = [SessionWrapper::inst().sess() sampleRate];
    break;
  case kAudioSessionProperty_PreferredHardwareIOBufferDuration: {
    assert(sizeof(Float32) == *ioDataSize);
    NSTimeInterval time = [SessionWrapper::inst().sess() IOBufferDuration];
    *(Float32 *)outData = time;
  } break;
  default:
    NSLog(@"Unsupported _AudioSessionGetProperty id %d", (unsigned int)inID);
    break;
  }
  return noErr;
}

OSStatus _AudioSessionSetProperty(AudioSessionPropertyID inID,
                                  UInt32 inDataSize, const void *inData) {
  assert(inData);
  switch (inID) {
  case kAudioSessionProperty_AudioCategory: {
    NSLog(@"kAudioSessionProperty_AudioCategory");
    // SessionWrapper.inst().sess.
    // NSString *p = [SessionWrapper::inst().sess() category];
    NSString *p = avsw_GetAVAudioSessionMode(*(UInt32 *)inData);
    [SessionWrapper::inst().sess() setCategory:p error:nil];

  } break;
  case kAudioSessionProperty_CurrentHardwareSampleRate: {
    assert(sizeof(Float64) == inDataSize);
    [SessionWrapper::inst().sess() setPreferredSampleRate:*(Float64 *)inData
                                                    error:nil];
  } break;
  case kAudioSessionProperty_PreferredHardwareIOBufferDuration: {
    assert(sizeof(UInt32) == inDataSize);
    [SessionWrapper::inst().sess()
        setPreferredIOBufferDuration:*(UInt32 *)inData
                               error:nil];
  } break;
  default:
    NSLog(@"Unsupported _AudioSessionSetProperty id %d", (unsigned int)inID);
    break;
  }
  return noErr;
}

} // extern "C"
