#include <cstdio>
#include <AudioToolbox/AudioToolbox.h>
#include "remoteio_mic_c.h"
//#include "DaTunerDebug.h"

#ifndef ASSERT
#define ASSERT(x) if (!(x)) {printf("\n***Assertion Failed at %s(%d)***\n", __FILE__, __LINE__); exit(-1);}
#endif
#ifndef ASSERT_FN
#define ASSERT_FN(x) if (!(x)) {printf("\n***Assertion Failed at %s(%d)***\n", __FILE__, __LINE__); exit(-1);}
#endif

#ifdef WIN32
#include <stdint.h>
#include <stdlib.h>
typedef int AudioStreamBasicDescription;
typedef int AudioUnit;
typedef int AudioBufferList;
typedef int OSStatus;
typedef int AudioUnitRenderActionFlags;
const int noErr = 0;
typedef uint32_t UInt32;
typedef int AudioComponentDescription;
#define TARGET_OS_IPHONE 1
#endif



typedef struct RemoteIO_Internal_tag { 
  RioInstance_t inst;

  AudioStreamBasicDescription myASBD; 
  //AUGraph graph; 
  AudioUnit inputUnit; 
  //AudioUnit outputUnit; 
  AudioBufferList *pInputBuffer; 
} RemoteIO_Internal_t;


static void _CheckError(OSStatus error, const char *operation) { 
  if (error == noErr) return; 
  char errorString[20]; // See if it appears to be a 4-char-code 
  *(UInt32 *)(errorString + 1) = CFSwapInt32HostToBig(error); 
  if (isprint(errorString[1]) && isprint(errorString[2]) && isprint(errorString[3]) && isprint(errorString[4])) { 
    errorString[0] = errorString[5] = '\''; 
    errorString[6] = '\0'; 
  } else // No, format it as an integer 
    sprintf(errorString, "%d", (int)error); 
  fprintf(stderr, "Error: %s (%s)\n", operation, errorString); 
  printf("Error: %s (%s)\n", operation, errorString); 
  exit(1); 
}

#define CheckError(cond, op) if ((cond) != noErr) { _CheckError( (cond), (op) ); }

extern "C" {

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
static OSStatus riom_input_render_proc(
  void *pInRefCon, 
  AudioUnitRenderActionFlags *pIoActionFlags, 
  const AudioTimeStamp *pInTimeStamp, 
  UInt32 inBusNumber, 
  UInt32 inNumberFrames, 
  AudioBufferList * pIoData) 
{ 
  RemoteIO_Internal_t *pPlayer = (RemoteIO_Internal_t*) pInRefCon;

  OSStatus inputProcErr = AudioUnitRender(pPlayer->inputUnit, pIoActionFlags, pInTimeStamp, inBusNumber, inNumberFrames, pPlayer->pInputBuffer);

  if (! inputProcErr) { 
    //inputProcErr = pPlayer->ringBuffer->Store(pPlayer->pInputBuffer, inNumberFrames, pInTimeStamp->mSampleTime); 
    if (pPlayer->inst.audCb) {
      pPlayer->inst.audCb( pPlayer->inst.pUserData, (float *)pPlayer->pInputBuffer->mBuffers[0].mData, 1, inNumberFrames );
    }
  }
  else {
      printf("AudioUnitRender returned -50.  pPlayer = 0x%x, pPlayer->pInputBuffer = 0x%x\n7||||||",
             (unsigned int)(uintptr_t)pPlayer,
             (unsigned int)(uintptr_t)pPlayer->pInputBuffer
             );
  }


  return inputProcErr; 
}

//#define USE_OLD

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
#if !TARGET_OS_IPHONE
static void riom_create_input_unit (RemoteIO_Internal_t *pPlayer) { 
  // Generates a description that matches audio HAL 
  AudioComponentDescription inputcd = {0}; 
  inputcd.componentType = kAudioUnitType_Output;
  inputcd.componentSubType = kAudioUnitSubType_HALOutput;
  inputcd.componentManufacturer = kAudioUnitManufacturer_Apple;

  AudioComponent comp = AudioComponentFindNext(NULL, &inputcd); 
  if (comp == NULL) { printf ("Can't get output unit"); exit (-1); } 

  CheckError(AudioComponentInstanceNew(comp, 
                                        &pPlayer->inputUnit), 
                                        "Couldn't open component for inputUnit");

  UInt32 disableFlag = 0; 
  UInt32 enableFlag = 1; 
  AudioUnitScope outputBus = 0; 
  AudioUnitScope inputBus = 1; 

  CheckError(AudioUnitSetProperty(pPlayer->inputUnit, 
                                  kAudioOutputUnitProperty_EnableIO, 
                                  kAudioUnitScope_Input, 
                                  inputBus, 
                                  &enableFlag, 
                                  sizeof(enableFlag)), 
                                  "Couldn't enable input on I/O unit"); 

  CheckError(AudioUnitSetProperty(pPlayer->inputUnit, 
                                  kAudioOutputUnitProperty_EnableIO,
                                  kAudioUnitScope_Output, 
                                  outputBus, 
                                  &disableFlag, 
                                  sizeof(enableFlag)), 
                                  "Couldn't disable output on I/O unit");

  AudioDeviceID defaultDevice = kAudioObjectUnknown;
  UInt32 propertySize = sizeof (defaultDevice); 

  AudioObjectPropertyAddress defaultDeviceProperty; 
  defaultDeviceProperty.mSelector = kAudioHardwarePropertyDefaultInputDevice; 
  defaultDeviceProperty.mScope = kAudioObjectPropertyScopeGlobal; 
  defaultDeviceProperty.mElement = kAudioObjectPropertyElementMaster; 

  CheckError(AudioObjectGetPropertyData(kAudioObjectSystemObject, 
                                        &defaultDeviceProperty, 
                                        0, 
                                        NULL, 
                                        &propertySize, 
                                        &defaultDevice), 
                                        "Couldn't get default input device");

  CheckError(AudioUnitSetProperty(pPlayer->inputUnit, 
                                  kAudioOutputUnitProperty_CurrentDevice, 
                                  kAudioUnitScope_Global, 
                                  outputBus, 
                                  &defaultDevice, 
                                  sizeof(defaultDevice)), 
                                  "Couldn't set default device on I/O unit");

  propertySize = sizeof (AudioStreamBasicDescription); 
  
  CheckError(AudioUnitGetProperty(pPlayer->inputUnit, 
                                  kAudioUnitProperty_StreamFormat, 
                                  kAudioUnitScope_Output, 
                                  inputBus, 
                                  &pPlayer->myASBD, 
                                  &propertySize), 
                                  "Couldn't get ASBD from output unit");

  AudioStreamBasicDescription myASBD; 
  CheckError(AudioUnitGetProperty(pPlayer->inputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, inputBus, &myASBD, &propertySize), "Couldn't get ASBD from input unit"); 

  pPlayer->myASBD.mSampleRate = myASBD.mSampleRate; 
  printf("Sample rate = %d\n", (int) myASBD.mSampleRate ); 

  pPlayer->inst.fs = (int) myASBD.mSampleRate;

  propertySize = sizeof (AudioStreamBasicDescription); 

  CheckError(AudioUnitSetProperty(pPlayer->inputUnit, 
                                  kAudioUnitProperty_StreamFormat, 
                                  kAudioUnitScope_Output, 
                                  inputBus, 
                                  &pPlayer->myASBD, 
                                  propertySize), 
                                  "Couldn't set ASBD on input unit");

  UInt32 bufferSizeFrames = 0; 
  propertySize = sizeof(UInt32); 

  CheckError (AudioUnitGetProperty(pPlayer->inputUnit, 
                                    kAudioDevicePropertyBufferFrameSize, 
                                    kAudioUnitScope_Global, 
                                    0, 
                                    &bufferSizeFrames, 
                                    &propertySize), 
                                    "Couldn't get buffer frame size from input unit"); 

  UInt32 bufferSizeBytes = bufferSizeFrames * sizeof(Float32);

  UInt32 propsize = offsetof(AudioBufferList, mBuffers[0]) + (sizeof(AudioBuffer) * pPlayer->myASBD.mChannelsPerFrame); 
  
  // malloc buffer lists 
  pPlayer->pInputBuffer = (AudioBufferList *)malloc(propsize); 

  printf("pPlayer->myASBD.mChannelsPerFrame = %d\n", pPlayer->myASBD.mChannelsPerFrame );
  pPlayer->pInputBuffer->mNumberBuffers = pPlayer->myASBD.mChannelsPerFrame; 
  
  // Pre-malloc buffers for AudioBufferLists 
  for(UInt32 i =0; i< pPlayer->pInputBuffer->mNumberBuffers ; i++) { 
    pPlayer->pInputBuffer->mBuffers[i].mNumberChannels = 1; 
    pPlayer->pInputBuffer->mBuffers[i].mDataByteSize = bufferSizeBytes; 
    pPlayer->pInputBuffer->mBuffers[i].mData = malloc(bufferSizeBytes); 
  }

  // Set render proc to supply samples from input unit 
  AURenderCallbackStruct callbackStruct; 
  callbackStruct.inputProc = riom_input_render_proc; 
  callbackStruct.inputProcRefCon = pPlayer; 
  CheckError(AudioUnitSetProperty(pPlayer->inputUnit, 
                                  kAudioOutputUnitProperty_SetInputCallback, 
                                  kAudioUnitScope_Global, 
                                  0, 
                                  &callbackStruct, 
                                  sizeof(callbackStruct)), 
                                  "Couldn't set input callback");

  CheckError(AudioUnitInitialize(pPlayer->inputUnit), 
                                 "Couldn't initialize input unit"); 

  printf ("Bottom of riom_create_input_unit()\n"); 
}
#else
static void riom_create_input_unit (RemoteIO_Internal_t *pPlayer) { 
  //const UInt32 kOutputBus = 0;
  const UInt32 kInputBus  = 1;
#if 0
  // We can't do this without self!
  // Set up the audio session 
  CheckError(AudioSessionInitialize(NULL, 
                                    kCFRunLoopDefaultMode, 
                                    MyInterruptionListener, 
                                    self), "Couldn't initialize the audio session"); 
#endif

#ifndef USE_OLD
  UInt32 category = kAudioSessionCategory_RecordAudio;
#else
  UInt32 category = kAudioSessionCategory_PlayAndRecord;
#endif
  CheckError(AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, 
                                    sizeof(category), &category), 
                                    "Couldn't set the category on the audio session");

  // Listing 10.20 Checking for audio input availability on iOS
  UInt32 ui32PropertySize = sizeof (UInt32); 
  UInt32 inputAvailable; 
  CheckError(AudioSessionGetProperty(kAudioSessionProperty_AudioInputAvailable, 
                                    &ui32PropertySize, 
                                    &inputAvailable), 
                                    "Couldn't get current audio input available prop"); 
 
  // Getting the hardware sample rate
  Float64 hardwareSampleRate; 
  UInt32 propSize = sizeof (hardwareSampleRate); 
  CheckError(AudioSessionGetProperty( kAudioSessionProperty_CurrentHardwareSampleRate, 
                                      &propSize, 
                                      &hardwareSampleRate), 
                                      "Couldn't get hardwareSampleRate"); 
  printf("hardwareSampleRate = %f\n", hardwareSampleRate);
  pPlayer->inst.fs = (int) hardwareSampleRate;

  // Getting RemoteIO AudioUnit from Audio Component Manager
  // Describe the unit 
  AudioComponentDescription audioCompDesc; 
  audioCompDesc.componentType = kAudioUnitType_Output; 
  audioCompDesc.componentSubType = kAudioUnitSubType_RemoteIO; 
  audioCompDesc.componentFlags = 0; 
  audioCompDesc.componentFlagsMask = 0; 
  audioCompDesc.componentManufacturer = kAudioUnitManufacturer_Apple; 
  
  // Get the RIO unit from the audio component manager 
  AudioComponent rioComponent = AudioComponentFindNext(NULL, &audioCompDesc); 
  CheckError(AudioComponentInstanceNew(rioComponent, 
                                        &pPlayer->inputUnit), 
                                        "Couldn't get RIO unit instance");

  // Set up the RIO unit for playback
  UInt32 oneFlag = 1; 
  AudioUnitElement bus0 = 0;
#ifdef USE_OLD
  CheckError(AudioUnitSetProperty (pPlayer->inputUnit,
                                    kAudioOutputUnitProperty_EnableIO, 
                                    kAudioUnitScope_Output, bus0, 
                                    &oneFlag, 
                                    sizeof(oneFlag)), 
                                    "Couldn't enable RIO output"); 
#endif
  // Enable RIO input
  AudioUnitElement bus1 = 1; 
  CheckError(AudioUnitSetProperty(pPlayer->inputUnit, 
                                    kAudioOutputUnitProperty_EnableIO, 
                                    kAudioUnitScope_Input, 
                                    bus1, 
                                    &oneFlag, 
                                    sizeof(oneFlag)), 
                                    "Couldn't enable RIO input");

  // Setup an ASBD in the iPhone canonical format 
  //AudioStreamBasicDescription myASBD;
  memset (&pPlayer->myASBD, 0, sizeof (pPlayer->myASBD));
  pPlayer->myASBD.mSampleRate = hardwareSampleRate;
  pPlayer->myASBD.mFormatID = kAudioFormatLinearPCM;
  const int numChannels = 1;
  const int bytesPerSample = sizeof(float);
  
#ifndef USE_OLD
  pPlayer->myASBD.mChannelsPerFrame = numChannels; // each frame is made up of 2 samples.
  pPlayer->myASBD.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
  pPlayer->myASBD.mFramesPerPacket = 1; // uncompressed audio, so 1
  pPlayer->myASBD.mBytesPerFrame = bytesPerSample * pPlayer->myASBD.mChannelsPerFrame;
  pPlayer->myASBD.mBytesPerPacket = pPlayer->myASBD.mBytesPerFrame * pPlayer->myASBD.mFramesPerPacket; // per buffer
  pPlayer->myASBD.mBitsPerChannel = bytesPerSample*8; // Set format for output (bus 0) on the RIO's input scope
#else
  pPlayer->myASBD.mChannelsPerFrame = numChannels; // each frame is made up of 2 samples.
  pPlayer->myASBD.mFormatFlags = kAudioFormatFlagsCanonical;
  pPlayer->myASBD.mBytesPerPacket = 4;
  pPlayer->myASBD.mFramesPerPacket = 1;
  pPlayer->myASBD.mBytesPerFrame = 4;
  pPlayer->myASBD.mBitsPerChannel = 16; // Set format for output (bus 0) on the RIO's input scope
#endif
  
  CheckError(AudioUnitSetProperty (pPlayer->inputUnit, 
                                    kAudioUnitProperty_StreamFormat,
                                    kAudioUnitScope_Input, 
                                    bus0, 
                                    &pPlayer->myASBD,
                                    sizeof (pPlayer->myASBD)), "Couldn't set the ASBD for RIO on input scope/bus 0");
  
  // Set ASBD for mic input (bus 1) on RIO's output scope
  CheckError(AudioUnitSetProperty (pPlayer->inputUnit, 
                                    kAudioUnitProperty_StreamFormat, 
                                    kAudioUnitScope_Output, 
                                    bus1, 
                                    &pPlayer->myASBD,
                                    sizeof (pPlayer->myASBD)), "Couldn't set the ASBD for RIO on output scope/bus 1"); //As you fill
    {
    UInt32 bufferSizeFrames = 1024;
    UInt32 bufferSizeBytes = bufferSizeFrames *  pPlayer->myASBD.mBytesPerFrame;
    /*CheckError (AudioUnitGetProperty(pPlayer->inputUnit,
                                     kAudioDevicePropertyBufferFrameSize,
                                     kAudioUnitScope_Global,
                                     0,
                                     &bufferSizeFrames,
                                     &propertySize),
                "Couldn't get buffer frame size from input unit");
    */
    // malloc buffer lists
    const int numBuffers = 1;
    pPlayer->pInputBuffer = (AudioBufferList *)malloc(sizeof(AudioBufferList) + ((numBuffers-1)*sizeof(AudioBuffer)));
    
    printf("pPlayer->myASBD.mChannelsPerFrame = %u\n", (unsigned int)pPlayer->myASBD.mChannelsPerFrame );
    pPlayer->pInputBuffer->mNumberBuffers = numBuffers;
    
    // Pre-malloc buffers for AudioBufferLists
    for(UInt32 i =0; i< pPlayer->pInputBuffer->mNumberBuffers ; i++) {
        pPlayer->pInputBuffer->mBuffers[i].mNumberChannels = 1;
        pPlayer->pInputBuffer->mBuffers[i].mDataByteSize = bufferSizeBytes;
        pPlayer->pInputBuffer->mBuffers[i].mData = malloc(bufferSizeBytes);
    }

   }
  
  // Set the callback method
  AURenderCallbackStruct callbackStruct; 
  callbackStruct.inputProc = riom_input_render_proc; 
  callbackStruct.inputProcRefCon = pPlayer; 
  CheckError(AudioUnitSetProperty(pPlayer->inputUnit, 
                                  kAudioOutputUnitProperty_SetInputCallback,
                                  kAudioUnitScope_Global, 
                                  0,
                                  &callbackStruct, 
                                  sizeof (callbackStruct)), 
                                  "Couldn't set RIO's render callback on bus 0");

  UInt32 zeroFlag = 0;
  CheckError(AudioUnitSetProperty(pPlayer->inputUnit, 
                                kAudioUnitProperty_ShouldAllocateBuffer,
                                kAudioUnitScope_Output, 
                                kInputBus,
                                &zeroFlag, 
                                sizeof(zeroFlag)), "Could not set to no allocation.");

  // Initialize and start the RIO unit 
  CheckError(AudioUnitInitialize(pPlayer->inputUnit), 
                              "Couldn't initialize the RIO unit"); 
  
  CheckError (AudioOutputUnitStart (pPlayer->inputUnit), 
                              "Couldn't start the RIO unit"); 
  
  printf("RIO started!\n"); 
  
  // Override point for customization after application launch. 
  //[self.window makeKeyAndVisible]; return YES; }
}
#endif


///////////////////////////////////////////////////////////////////////////////////////////////////
// Starts the microphone, which will start triggering callbacks.
///////////////////////////////////////////////////////////////////////////////////////////////////
RioInstance_t * rio_start_mic( 
    void * const pSelf,
    void * const pUserData,
    fnAudioCallbackFn fnPtr)
{

  RemoteIO_Internal_t *pInst = (RemoteIO_Internal_t *)malloc(sizeof(RemoteIO_Internal_t) );

  ASSERT( NULL != pInst );
  memset( pInst, 0, sizeof( RemoteIO_Internal_t ) );
  pInst->inst.pUserData = pUserData;
  pInst->inst.audCb = fnPtr;

  riom_create_input_unit(pInst);

  CheckError(AudioOutputUnitStart(pInst->inputUnit), "AudioOutputUnitStart failed");
#if TARGET_IOS_IPHONE
  CheckError(AudioSessionSetActive(true), "Couldn't re-set audio session active");
#endif
  return &pInst->inst;

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
void rio_stop_mic( RioInstance_t *pInst ) 
{
  RemoteIO_Internal_t *pPlayer = (RemoteIO_Internal_t *)pInst;

  AudioOutputUnitStop(pPlayer->inputUnit); 
  AudioUnitUninitialize (pPlayer->inputUnit); 

  // Free all allocated buffers.
  for (int i = 0; i < pPlayer->pInputBuffer->mNumberBuffers; i++) {
    free( pPlayer->pInputBuffer->mBuffers[i].mData );
    pPlayer->pInputBuffer->mBuffers[i].mData = NULL;
  }
  
  // Free the input buffer structure.
  free(pPlayer->pInputBuffer);
  pPlayer->pInputBuffer = NULL;

  // Fini!

}


} // extern "C"
