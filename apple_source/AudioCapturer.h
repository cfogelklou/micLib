#import <Foundation/Foundation.h>

@interface AudioCapturer : NSObject
- (instancetype)initWithSampleRate:(float)sampleRate callback:(void(^)(float *, int))callback;
- (void)startCapture;
- (void)stopCapture;
@end