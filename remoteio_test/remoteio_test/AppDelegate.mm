//
//  AppDelegate.m
//  remoteio_test
//
//  Created by Chris Fogelklou on 28/05/15.
//  Copyright (c) 2015 Applicaudia. All rights reserved.
//

#import "AppDelegate.h"
#import <AudioToolbox/AudioToolbox.h>

#import "AudioCapturer.h"


@interface AppDelegate ()

@property (nonatomic, strong) AudioCapturer *audioCapturer;

@end

@implementation AppDelegate

struct UserData {
  double fs;
};

static struct UserData g_UserData;

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
  float sampleRate = 44100.0;
  g_UserData.fs = sampleRate;
  self.audioCapturer = [[AudioCapturer alloc] initWithSampleRate:sampleRate callback:^(void *userData, float *data, int numFrames) {
      struct UserData *ud = (struct UserData *)userData;
      assert(ud->fs == sampleRate);
      for (int i = 0; i < numFrames; i++) {
          float sample = data[i];
          NSLog(@"Sample: %f", sample);
      }
      // Use ud->... to access the custom data
  } userData:&g_UserData];

    // Start capturing audio
    [self.audioCapturer startCapture];
    
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application {
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
    // Stop capturing audio when the app goes to background
    [self.audioCapturer stopCapture];    
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
    // Start capturing audio when the app becomes active
    [self.audioCapturer startCapture];    
}

- (void)applicationWillTerminate:(UIApplication *)application {
  // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
  // Stop capturing audio when the app goes to background
  [self.audioCapturer stopCapture];

}

@end
