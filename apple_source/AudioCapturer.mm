//
//  AudioCapturer.mm
//  MicrophoneAudioCapturer
//
//  The AudioCapturer class is responsible for setting up and managing an AVAudioEngine
//  instance to capture audio from the microphone, convert it to a mono float stream,
//  and pass it to the user-provided callback function.
//
//  The AudioCapturer also provides a set of C-callable wrapper functions that allow
//  the class to be easily integrated into C and C++ projects.
//

#import "AudioCapturer.h"
#import <AVFoundation/AVFoundation.h>


@implementation AudioCapturer {
    AVAudioEngine *_engine;
    float _requestedSampleRate;
    float _actualSampleRate;
    void (^_callback)(void *, float *, int);
    void *_userData;
}

- (instancetype)initWithSampleRate:(float)sampleRate callback:(void(^)(void *, float *, int))callback userData:(void *)userData {
    if (self = [super init]) {
        _requestedSampleRate = sampleRate;
        _actualSampleRate = 0.0;
        _callback = callback;
        _userData = userData;
        [self setupAudioEngine];
    }
    return self;
}

- (void)setupAudioEngine {
    _engine = [[AVAudioEngine alloc] init];

    AVAudioSession *audioSession = [AVAudioSession sharedInstance];
    [audioSession setCategory:AVAudioSessionCategoryRecord error:nil];
    [audioSession setPreferredSampleRate:_requestedSampleRate error:nil];
    [audioSession setActive:YES error:nil];

    AVAudioInputNode *inputNode = _engine.inputNode;
    AVAudioFormat *inputFormat = [inputNode outputFormatForBus:0];
    
    // Set up the format for mono audio with a single float sample
    AVAudioFormat *monoFormat = [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32 sampleRate:inputFormat.sampleRate channels:1 interleaved:NO];

    _actualSampleRate = [_engine.inputNode inputFormatForBus:0].sampleRate;

    [inputNode installTapOnBus:0 bufferSize:1024 format:monoFormat block:^(AVAudioPCMBuffer *buffer, AVAudioTime *when) {
        float *data = buffer.floatChannelData[0];
        int numFrames = (int)buffer.frameLength;
        self->_callback(self->_userData, data, numFrames);
    }];
}

- (void)startCapture {
    [_engine prepare];
    NSError *error = nil;
    if (![_engine startAndReturnError:&error]) {
        NSLog(@"Error starting AVAudioEngine: %@", error.localizedDescription);
    }
}

- (void)stopCapture {
    [_engine stop];
}

- (float)getActualSampleRate {
    return _actualSampleRate;
}

@end

extern "C" {

// C-callable wrapper functions

void *AudioCapturer_create(float sampleRate, void (*callback)(void *, float *, int), void *userData) {
    AudioCapturer *audioCapturer = [[AudioCapturer alloc] initWithSampleRate:sampleRate callback:^(void *userData, float *data, int numFrames) {
        callback(userData, data, numFrames );
    } userData:userData];
    return (__bridge_retained void *)audioCapturer;
}

void AudioCapturer_startCapture(void *audioCapturer) {
    AudioCapturer *capturer = (__bridge AudioCapturer *)audioCapturer;
    [capturer startCapture];
}

void AudioCapturer_stopCapture(void *audioCapturer) {
    AudioCapturer *capturer = (__bridge AudioCapturer *)audioCapturer;
    [capturer stopCapture];
}

void AudioCapturer_destroy(void *audioCapturer) {
    AudioCapturer *capturer = (__bridge_transfer AudioCapturer *)audioCapturer;
    capturer = nil;
}

float AudioCapturer_getActualSampleRate(void *audioCapturer) {
    AudioCapturer *capturer = (__bridge AudioCapturer *)audioCapturer;
    return [capturer getActualSampleRate];
}

}
