#import "AudioCapturer.h"
#import <AVFoundation/AVFoundation.h>


@implementation AudioCapturer {
    AVAudioEngine *_engine;
    float _sampleRate;
    void (^_callback)(void *, float *, int);
    void *_userData;
}

- (instancetype)initWithSampleRate:(float)sampleRate callback:(void(^)(void *, float *, int))callback userData:(void *)userData {
    if (self = [super init]) {
        _sampleRate = sampleRate;
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
    [audioSession setPreferredSampleRate:_sampleRate error:nil];
    [audioSession setActive:YES error:nil];

    AVAudioInputNode *inputNode = _engine.inputNode;
    AVAudioFormat *inputFormat = [inputNode outputFormatForBus:0];
    
    // Set up the format for mono audio with a single float sample
    AVAudioFormat *monoFormat = [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32 sampleRate:inputFormat.sampleRate channels:1 interleaved:NO];

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

}
