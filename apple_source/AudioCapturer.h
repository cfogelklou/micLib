#import <Foundation/Foundation.h>

@interface AudioCapturer : NSObject
- (instancetype)initWithSampleRate:(float)sampleRate callback:(void(^)(void *, float *, int))callback userData:(void *)userData;
- (void)startCapture;
- (void)stopCapture;
@end

