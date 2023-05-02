#import "AudioCapturer.h"
#import <AVFoundation/AVFoundation.h>


@implementation AudioCapturer {
    AVAudioEngine *_engine;
    float _sampleRate;
    void (^_callback)(float *, int);
}

- (instancetype)initWithSampleRate:(float)sampleRate callback:(void(^)(float *, int))callback {
    if (self = [super init]) {
        _sampleRate = sampleRate;
        _callback = callback;
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
        _callback(data, numFrames);
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
