//
//  AudioCapturer.h
//  MicrophoneAudioCapturer
//
//  The AudioCapturer class is responsible for setting up and managing an AVAudioEngine
//  instance to capture audio from the microphone, convert it to a mono float stream,
//  and pass it to the user-provided callback function.
//
//  The AudioCapturer also provides a set of C-callable wrapper functions that allow
//  the class to be easily integrated into C and C++ projects.
//

#import <Foundation/Foundation.h>

@interface AudioCapturer : NSObject
- (instancetype)initWithSampleRate:(float)sampleRate callback:(void(^)(void *, float *, int))callback userData:(void *)userData;
- (void)startCapture;
- (void)stopCapture;
@end

