#include <cstdio>
#include <math.h>
#include <cstddef>
#include <AudioToolbox/AudioToolbox.h>

#include "remoteio_mic_c.h"

#define RIOTRACE(x) printf x

#ifndef ASSERT
#define ASSERT(x) if (!(x)) do{printf("\n***Assertion Failed: %s(%d):%s\n", __FILE__, __LINE__, #x); return false;}while(0)
#endif
#ifndef ASSERT_FN
#define ASSERT_FN(x) if (!(x)) do{printf("\n***Assertion Failed:  %s(%d):%s\n", __FILE__, __LINE__, #x); return false;}while(0)
#endif
#ifndef ASSERT_AT_COMPILE_TIME
#define ASSERT_AT_COMPILE_TIME(pred) switch(0){case 0:break;case (pred):break;;}
#endif

#define CheckError(cond, op) if ((cond) != noErr) do{printf("\n***CheckError Failed: %s(%d):%s\n", __FILE__, __LINE__, #cond); return false;}while(0)

typedef struct RemoteIO_Internal_tag { 
  RioInstance_t inst;

  AudioStreamBasicDescription myASBD; 
  //AUGraph graph; 
  AudioUnit inputUnit; 
  //AudioUnit outputUnit; 
  AudioBufferList *pInputBuffer; 

  AURenderCallbackStruct oldRenderCallbackStruct;
} RemoteIO_Internal_t;




extern "C" {

static const char *riomGetUintStr(UInt32 chars) {
    static char bytesArr[8][8];
    static int bytesIdx = 0;
    bytesIdx = (bytesIdx + 1) & (8 - 1);
    char *pBytes = bytesArr[bytesIdx];
    pBytes[0] = 0xff & (chars >> 24);
    pBytes[1] = 0xff & (chars >> 16);
    pBytes[2] = 0xff & (chars >> 8);
    pBytes[3] = 0xff & (chars >> 0);
    pBytes[4] = 0;
    return pBytes;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
static bool riom_print_asbd(const char * const szHeader, const AudioStreamBasicDescription * const pAsbd) {

    RIOTRACE(("%s:\n", szHeader));
    RIOTRACE(("  pAsbd->mChannelsPerFrame = %u\n", (unsigned int)pAsbd->mChannelsPerFrame));
    RIOTRACE(("  pAsbd->mFormatFlags = 0x%x\n", (unsigned int)pAsbd->mFormatFlags));
    RIOTRACE(("  pAsbd->mFramesPerPacket = %u\n", (unsigned int)pAsbd->mFramesPerPacket));
    RIOTRACE(("  pAsbd->mBytesPerFrame = %u\n", (unsigned int)pAsbd->mBytesPerFrame));
    RIOTRACE(("  pAsbd->mBytesPerPacket = %u\n", (unsigned int)pAsbd->mBytesPerPacket));
    RIOTRACE(("  pAsbd->mBitsPerChannel = %u\n", (unsigned int)pAsbd->mBitsPerChannel));
    return true;
}


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
      RIOTRACE(("AudioUnitRender returned %d.  pPlayer = 0x%x, pPlayer->pInputBuffer = 0x%x\n",
        (int)inputProcErr,
             (unsigned int)(uintptr_t)pPlayer,
             (unsigned int)(uintptr_t)pPlayer->pInputBuffer
        ));
  }


  return inputProcErr; 
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
#if !TARGET_OS_IPHONE
static bool riom_create_input_unit (RemoteIO_Internal_t *pPlayer) { 
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

  CheckError(AudioOutputUnitStart(pPlayer->inputUnit), "AudioOutputUnitStart failed");

  return true;

}
#else

///////////////////////////////////////////////////////////////////////////////////////////////////
// We want to ensure that Unity has enabled a mode that allows recording.
///////////////////////////////////////////////////////////////////////////////////////////////////
static bool riom_set_record_category(RemoteIO_Internal_t *pPlayer)
{
    const AudioSessionPropertyID id= kAudioSessionProperty_AudioCategory;
    UInt32 category = 0;
    UInt32 propSize = sizeof(category);
    ASSERT_FN(noErr == AudioSessionGetProperty(id, &propSize, &category));
    if ((category != kAudioSessionCategory_RecordAudio) && (category != kAudioSessionCategory_PlayAndRecord)) {
        RIOTRACE(("Category was %s, now set to %s\n", riomGetUintStr(category), riomGetUintStr(kAudioSessionCategory_PlayAndRecord)));
        category = kAudioSessionCategory_PlayAndRecord;
        ASSERT_FN(noErr == AudioSessionSetProperty(id, sizeof(category), &category));
    }
    ASSERT_FN(noErr == AudioSessionGetProperty(id, &propSize, &category));
    return ((category == kAudioSessionCategory_RecordAudio) || (category == kAudioSessionCategory_PlayAndRecord));

failure:
    return false;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
/*
*
*                         -------------------------
*                         | i                   o |
*-- BUS 1 -- from mic --> | n    REMOTE I/O     u | -- BUS 1 -- to app -->
*                         | p      AUDIO        t |
*-- BUS 0 -- from app --> | u       UNIT        p | -- BUS 0 -- to speaker -->
*                         | t                   u |
*                         |                     t |
*                         -------------------------
*Ergo, the stream properties for this unit are
*              Bus 0                               Bus 1
*Input Scope:  Set ASBD to indicate what           Get ASBD to inspect audio
*              you’re providing for play-out	    format being received from H/W
*
*Output Scope:	Get ASBD to inspect audio format    Set ASBD to indicate what format
*               being sent to H/W	                you want your units to receive.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////
static bool riom_set_stream_parameters(RemoteIO_Internal_t *pPlayer, const Float64 hardwareSampleRate) {
    const AudioUnitElement kOutputBus0 = 0;
    const AudioUnitElement kInputBus1 = 1;
    // Get properties, and set them if necessary.
    AudioStreamBasicDescription asbdExpected;
    memset(&asbdExpected, 0, sizeof(asbdExpected));
    memset(&pPlayer->myASBD, 0, sizeof(pPlayer->myASBD));
    UInt32 propSize = sizeof(pPlayer->myASBD);

    ASSERT_FN(noErr == AudioUnitGetProperty(pPlayer->inputUnit,
        kAudioUnitProperty_StreamFormat,
        kAudioUnitScope_Input,
        kOutputBus0,
        &pPlayer->myASBD,
        &propSize));

    riom_print_asbd("kAudioUnitScope_Input, kOutputBus0", &pPlayer->myASBD);

    //hardwareSampleRate = 24000.000000;
    //pPlayer->myASBD.mChannelsPerFrame = 2;
    //pPlayer->myASBD.mFormatFlags = 0x29;
    //pPlayer->myASBD.mFramesPerPacket = 1;
    //pPlayer->myASBD.mBytesPerFrame = 4;
    //pPlayer->myASBD.mBytesPerPacket = 4;
    //pPlayer->myASBD.mBitsPerChannel = 32;

    ASSERT_AT_COMPILE_TIME((kAudioFormatFlagIsNonInterleaved | kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked) == 0x29);
    bool propsAsExpected = true;
    asbdExpected.mChannelsPerFrame = 2;
    asbdExpected.mFormatFlags = 
        kAudioFormatFlagIsNonInterleaved | kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;// 0x29;
    asbdExpected.mFramesPerPacket = 1;
    asbdExpected.mBytesPerFrame = 4;
    asbdExpected.mBytesPerPacket = 4;
    asbdExpected.mBitsPerChannel = 32;

    propsAsExpected &= (asbdExpected.mChannelsPerFrame == pPlayer->myASBD.mChannelsPerFrame);
    propsAsExpected &= (asbdExpected.mFormatFlags == pPlayer->myASBD.mFormatFlags);
    propsAsExpected &= (asbdExpected.mFramesPerPacket == pPlayer->myASBD.mFramesPerPacket);
    propsAsExpected &= (asbdExpected.mBytesPerFrame == pPlayer->myASBD.mBytesPerFrame);
    propsAsExpected &= (asbdExpected.mBytesPerPacket == pPlayer->myASBD.mBytesPerPacket);
    propsAsExpected &= (asbdExpected.mBitsPerChannel == pPlayer->myASBD.mBitsPerChannel);
    if (!propsAsExpected) {
        RIOTRACE(("Not all ASBD parameters matched expectations!, adjusting\n"));
        ASSERT_FN(noErr == AudioUnitSetProperty(pPlayer->inputUnit,
            kAudioUnitProperty_StreamFormat,
            kAudioUnitScope_Input,
            kOutputBus0,
            &asbdExpected,
            propSize));
        pPlayer->myASBD = asbdExpected;
    }

    

    return propsAsExpected;

}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
static bool riom_create_input_unit(RemoteIO_Internal_t *pPlayer) {
    const AudioUnitElement kOutputBus0 = 0;
    const AudioUnitElement kInputBus1 = 1;
    // Set up the RIO unit for playback
    const UInt32 kOneFlag = 1;
    Float64 hardwareSampleRate = 0;
    Float32 hardwareDurationSeconds = 0;
    UInt32  bufferSizeFrames = 0;

    // Check to ensure we are using a category for recording.
    ASSERT_FN(riom_set_record_category(pPlayer));

    // Figure out if input hardware is available.
    {
        UInt32 inputAvailable = 0;
        UInt32 propSize = sizeof(inputAvailable);
        ASSERT_FN(noErr == AudioSessionGetProperty(kAudioSessionProperty_AudioInputAvailable, &propSize, &inputAvailable));
        ASSERT(0 != inputAvailable);
        RIOTRACE(("inputAvailable = %u\n", (unsigned int)inputAvailable));
    }
 
  // Getting the hardware sample rate
    {
  UInt32 propSize = sizeof (hardwareSampleRate); 
        ASSERT_FN(noErr == AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareSampleRate, &propSize, &hardwareSampleRate));
        if (hardwareSampleRate < 8000) {
            hardwareSampleRate = 24000;
            RIOTRACE(("Hardware sample rate not set.  Setting to %d\n", (int)hardwareSampleRate));
            
            ASSERT_FN(noErr == AudioSessionSetProperty(kAudioSessionProperty_CurrentHardwareSampleRate,
                propSize,
                &hardwareSampleRate));

            ASSERT_FN(noErr == AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareSampleRate, &propSize, &hardwareSampleRate));
        }
        RIOTRACE(("hardwareSampleRate = %f\n", hardwareSampleRate));

        pPlayer->inst.fs = (int)hardwareSampleRate;
    }

    // Getting the preferred buffer size...
    {
        UInt32 propSize = sizeof(hardwareDurationSeconds);
        ASSERT_FN(noErr == AudioSessionGetProperty(
            kAudioSessionProperty_PreferredHardwareIOBufferDuration, 
                                      &propSize, 
            &hardwareDurationSeconds));
        
        if (hardwareDurationSeconds <= 0) {
            bufferSizeFrames = 256;
            hardwareDurationSeconds = bufferSizeFrames / hardwareSampleRate;
            RIOTRACE(("Invalid hardwareDurationSeconds, setting to = %f ms\n", 1000.0f * hardwareDurationSeconds));
        }
        else {
            RIOTRACE(("hardwareDurationSeconds = %f ms\n", 1000.0f * hardwareDurationSeconds));
            Float32 fSamps = hardwareDurationSeconds * hardwareSampleRate;
            RIOTRACE(("approximate buffer frames = %f\n", fSamps));
            Float32 fSampsLog2 = logf(fSamps) / logf(2);
            fSampsLog2 = ceil(fSampsLog2);
            bufferSizeFrames = powf(2, fSampsLog2);
            RIOTRACE(("actual buffer frames = %d\n", (int)bufferSizeFrames));
        }

    }

  // Getting RemoteIO AudioUnit from Audio Component Manager
  // Describe the unit 
    {
  AudioComponentDescription audioCompDesc; 
        memset(&audioCompDesc, 0, sizeof(audioCompDesc));
  audioCompDesc.componentType = kAudioUnitType_Output; 
  audioCompDesc.componentSubType = kAudioUnitSubType_RemoteIO; 
  audioCompDesc.componentManufacturer = kAudioUnitManufacturer_Apple; 
  
  // Get the RIO unit from the audio component manager 
        RIOTRACE(("Getting input component...\n"));
  AudioComponent rioComponent = AudioComponentFindNext(NULL, &audioCompDesc); 
        ASSERT_FN(noErr == AudioComponentInstanceNew(rioComponent, &pPlayer->inputUnit));
        RIOTRACE(("Got component!"));
    }

    // Stop the hardware so we can initialize some stuff.
    ASSERT_FN(noErr == AudioOutputUnitStop(pPlayer->inputUnit));

    // Set the stream parameters!
    ASSERT_FN( riom_set_stream_parameters(pPlayer, hardwareSampleRate));

    // Enable microphone hardware if not already enabled!
    {
        UInt32 micHwEnabled = 0;
        UInt32 flagSize = sizeof(micHwEnabled);

        // First get status.
        ASSERT_FN(noErr == AudioUnitGetProperty(pPlayer->inputUnit,
                                    kAudioOutputUnitProperty_EnableIO, 
            kAudioUnitScope_Input,
            kInputBus1,
            &micHwEnabled,
            &flagSize));
        
        RIOTRACE(("micHwEnabled = %u\n", (unsigned int)micHwEnabled));
        
        // If disabled, then enable it.
        if (micHwEnabled == 0) {
            RIOTRACE(("Enabling microphone hardware.\n"));
            ASSERT_FN(noErr == AudioUnitSetProperty(pPlayer->inputUnit,
                                    kAudioOutputUnitProperty_EnableIO, 
                                    kAudioUnitScope_Input, 
                kInputBus1,
                &kOneFlag,
                sizeof(kOneFlag)));
        }
    }

    // Do some logging so we know the settings used.
    {
        AudioStreamBasicDescription asbd;
        UInt32 propSize = sizeof(asbd);
        memset(&asbd, 0, propSize);

        ASSERT_FN(noErr == AudioUnitGetProperty(pPlayer->inputUnit,
            kAudioUnitProperty_StreamFormat,
            kAudioUnitScope_Input,
            kOutputBus0,
            &asbd,
            &propSize));

        riom_print_asbd("kAudioUnitScope_Input,kOutputBus0(spkr)", &asbd);

        ASSERT_FN(noErr == AudioUnitGetProperty(pPlayer->inputUnit,
            kAudioUnitProperty_StreamFormat,
            kAudioUnitScope_Output,
            kInputBus1,
            &asbd,
            &propSize));

        riom_print_asbd("kAudioUnitScope_Output,kInputBus1(mic)", &asbd);

    }

#if 0
  // Setup an ASBD in the iPhone canonical format 
  //AudioStreamBasicDescription myASBD;
  memset (&pPlayer->myASBD, 0, sizeof (pPlayer->myASBD));
  pPlayer->myASBD.mSampleRate = hardwareSampleRate;
  pPlayer->myASBD.mFormatID = kAudioFormatLinearPCM;
  const int numChannels = 1;
  const int bytesPerSample = sizeof(float);
  
  pPlayer->myASBD.mChannelsPerFrame = numChannels; // each frame is made up of 2 samples.
  pPlayer->myASBD.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
  pPlayer->myASBD.mFramesPerPacket = 1; // uncompressed audio, so 1
  pPlayer->myASBD.mBytesPerFrame = bytesPerSample * pPlayer->myASBD.mChannelsPerFrame;
  pPlayer->myASBD.mBytesPerPacket = pPlayer->myASBD.mBytesPerFrame * pPlayer->myASBD.mFramesPerPacket; // per buffer
  pPlayer->myASBD.mBitsPerChannel = bytesPerSample*8; // Set format for output (bus 0) on the RIO's input scope


  
  CheckError(AudioUnitSetProperty (pPlayer->inputUnit, 
                                    kAudioUnitProperty_StreamFormat,
                                    kAudioUnitScope_Input, 
        kOutputBus0,
                                    &pPlayer->myASBD,
                                    sizeof (pPlayer->myASBD)), "Couldn't set the ASBD for RIO on input scope/bus 0");
  
  // Set ASBD for mic input (bus 1) on RIO's output scope
  CheckError(AudioUnitSetProperty (pPlayer->inputUnit, 
                                    kAudioUnitProperty_StreamFormat, 
                                    kAudioUnitScope_Output, 
        kInputBus1,
                                    &pPlayer->myASBD,
                                    sizeof (pPlayer->myASBD)), "Couldn't set the ASBD for RIO on output scope/bus 1"); //As you fill
#endif
    {
    UInt32 bufferSizeBytes = bufferSizeFrames *  pPlayer->myASBD.mBytesPerFrame;
    /*CheckError (AudioUnitGetProperty(pPlayer->inputUnit,
                                     kAudioDevicePropertyBufferFrameSize,
                                     kAudioUnitScope_Global,
                                     0,
                                     &bufferSizeFrames,
                                     &propertySize),
                "Couldn't get buffer frame size from input unit");
    */
        const int numBuffers = pPlayer->myASBD.mChannelsPerFrame;
        ASSERT(numBuffers >= 1);
        
        // This code will ONLY work if non-interleaved is set, so there are independent buffers for L and R.
        ASSERT((1 == numBuffers) || (pPlayer->myASBD.mFormatFlags & kAudioFormatFlagIsNonInterleaved));

    // malloc buffer lists
    pPlayer->pInputBuffer = (AudioBufferList *)malloc(sizeof(AudioBufferList) + ((numBuffers-1)*sizeof(AudioBuffer)));
    
        RIOTRACE(("pPlayer->myASBD.mChannelsPerFrame = %u\n", (unsigned int)pPlayer->myASBD.mChannelsPerFrame));
    pPlayer->pInputBuffer->mNumberBuffers = numBuffers;
    
    // Pre-malloc buffers for AudioBufferLists
    for(UInt32 i =0; i< pPlayer->pInputBuffer->mNumberBuffers ; i++) {
        pPlayer->pInputBuffer->mBuffers[i].mNumberChannels = 1;
        pPlayer->pInputBuffer->mBuffers[i].mDataByteSize = bufferSizeBytes;
        pPlayer->pInputBuffer->mBuffers[i].mData = malloc(bufferSizeBytes);
    }
    }

    // Set a new callback struct, but save the old one first.
    {
        UInt32 propSize = sizeof(pPlayer->oldRenderCallbackStruct);
        memset(&pPlayer->oldRenderCallbackStruct, 0, sizeof(pPlayer->oldRenderCallbackStruct));
        
        ASSERT_FN(noErr == AudioUnitGetProperty(pPlayer->inputUnit,
            kAudioOutputUnitProperty_SetInputCallback,
            kAudioUnitScope_Global,
            0,
            &pPlayer->oldRenderCallbackStruct,
            &propSize));

        RIOTRACE(("Saving the old callback = 0x%x\n", (unsigned int)(uintptr_t)pPlayer->oldRenderCallbackStruct.inputProc));

  
        // Set the callback method to point to our new render method.
  AURenderCallbackStruct callbackStruct; 
  callbackStruct.inputProc = riom_input_render_proc; 
  callbackStruct.inputProcRefCon = pPlayer; 
        ASSERT_FN(noErr == AudioUnitSetProperty(pPlayer->inputUnit,
                                  kAudioOutputUnitProperty_SetInputCallback,
                                  kAudioUnitScope_Global, 
                                  0,
                                  &callbackStruct, 
            sizeof(callbackStruct)));

    }

  // Initialize and start the RIO unit 
    ASSERT_FN(noErr == AudioUnitInitialize(pPlayer->inputUnit));
    ASSERT_FN(noErr == AudioOutputUnitStart(pPlayer->inputUnit));
  
    RIOTRACE(("RIO started!\n"));
  
    return true;
  
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

  if (NULL == pInst) return NULL;
  memset( pInst, 0, sizeof( RemoteIO_Internal_t ) );
  pInst->inst.pUserData = pUserData;
  pInst->inst.audCb = fnPtr;

  riom_create_input_unit(pInst);

  
#if 0//TARGET_IOS_IPHONE
  CheckError(AudioSessionSetActive(true), "Couldn't re-set audio session active");
#endif
  return &pInst->inst;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
static bool rio_restore_old_callback(RemoteIO_Internal_t *pPlayer){

    ASSERT_FN(noErr == AudioUnitSetProperty(pPlayer->inputUnit,
        kAudioOutputUnitProperty_SetInputCallback,
        kAudioUnitScope_Global,
        0,
        &pPlayer->oldRenderCallbackStruct,
        sizeof(pPlayer->oldRenderCallbackStruct)));

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
void rio_stop_mic(RioInstance_t *pInst) {
  RemoteIO_Internal_t *pPlayer = (RemoteIO_Internal_t *)pInst;

  if (pPlayer == NULL)
    return;

  AudioOutputUnitStop(pPlayer->inputUnit);
  rio_restore_old_callback(pPlayer);

  AudioUnitUninitialize(pPlayer->inputUnit);
  if (pPlayer->pInputBuffer) {
    // Free all allocated buffers.
    for (int i = 0; i < pPlayer->pInputBuffer->mNumberBuffers; i++) {
      if (pPlayer->pInputBuffer->mBuffers[i].mData) {
        free(pPlayer->pInputBuffer->mBuffers[i].mData);
      }
      pPlayer->pInputBuffer->mBuffers[i].mData = NULL;
    }

    // Free the input buffer structure.
    free(pPlayer->pInputBuffer);
  }
  pPlayer->pInputBuffer = NULL;

  // Fini!
  free(pPlayer);
}

} // extern "C"
