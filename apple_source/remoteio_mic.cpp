#include <AudioToolbox/AudioToolbox.h>
#include <cstddef>
#include <cstdio>
#include <math.h>

#include "remoteio_mic_c.h"

#define RIOTRACE(x) printf x

static const char * rio_lastErrorFile = "";
static int rio_lastErrorLine = -1;

static void rio_AssertionFailed(const char *file, const int line, const char *comparison){
  printf("\n***Assertion Failed: %s(%d):%s\n", file, line, comparison);
  if (rio_lastErrorLine < 0){
    rio_lastErrorLine = line;
    rio_lastErrorFile = file;
  }

  assert(false);
}

const AudioUnitElement kOutputBus0 = 0;
const AudioUnitElement kInputBus1 = 1;


#ifndef LOG_ASSERT_FN
#define LOG_ASSERT_FN(x) do { \
    if (!(x)) { \
        rio_AssertionFailed(__FILE__, __LINE__, #x); \
    } \
  } while (0)
#endif

#ifndef LOG_ASSERT
#define LOG_ASSERT(x) LOG_ASSERT_FN(x)
#endif

#ifndef ASSERT_AT_COMPILE_TIME
#define ASSERT_AT_COMPILE_TIME(pred)                                           \
  switch (0) {                                                                 \
  case 0:                                                                      \
    break;                                                                     \
  case (pred):                                                                 \
    break;                                                                     \
    ;                                                                          \
  }
#endif

#define CheckError(cond, op)                                                   \
  if ((cond) != noErr)                                                         \
    do {                                                                       \
      printf("\n***CheckError Failed: %s(%d):%s\n", __FILE__, __LINE__,        \
             #cond);                                                           \
      return false;                                                            \
  } while (0)

#define DEFAULT_BUFFER_SIZE_FRAMES 512


// ////////////////////////////////////////////////////////////////////////////
typedef struct RemoteIO_Internal_tag {
  RioInstance_t inst;

  AudioStreamBasicDescription myASBD;
  // AUGraph graph;
  AudioUnit inputUnit;
  // AudioUnit outputUnit;
  AudioBufferList *pInputBuffer;

  AURenderCallbackStruct oldRenderCallbackStruct;

  // If the user wants a specific FS, allows it to be passed in.
  int desiredFs;
} RemoteIO_Internal_t;

extern "C" {

// ////////////////////////////////////////////////////////////////////////////
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
// ////////////////////////////////////////////////////////////////////////////
static bool riom_print_asbd(const char *const szHeader,
                            const AudioStreamBasicDescription *const pAsbd) {

  RIOTRACE(("%s:\n", szHeader));
  RIOTRACE(("  pAsbd->mChannelsPerFrame = %u\n",
            (unsigned int)pAsbd->mChannelsPerFrame));
  RIOTRACE(
      ("  pAsbd->mFormatFlags = 0x%x\n", (unsigned int)pAsbd->mFormatFlags));
  RIOTRACE(("  pAsbd->mFramesPerPacket = %u\n",
            (unsigned int)pAsbd->mFramesPerPacket));
  RIOTRACE(
      ("  pAsbd->mBytesPerFrame = %u\n", (unsigned int)pAsbd->mBytesPerFrame));
  RIOTRACE(("  pAsbd->mBytesPerPacket = %u\n",
            (unsigned int)pAsbd->mBytesPerPacket));
  RIOTRACE(("  pAsbd->mBitsPerChannel = %u\n",
            (unsigned int)pAsbd->mBitsPerChannel));
  return true;
}

// ////////////////////////////////////////////////////////////////////////////
static OSStatus
riom_input_render_proc(void *pInRefCon,
                       AudioUnitRenderActionFlags *pIoActionFlags,
                       const AudioTimeStamp *pInTimeStamp, UInt32 inBusNumber,
                       UInt32 inNumberFrames, AudioBufferList *pIoData) {
  RemoteIO_Internal_t *pPlayer = (RemoteIO_Internal_t *)pInRefCon;

  OSStatus inputProcErr =
      AudioUnitRender(pPlayer->inputUnit, pIoActionFlags, pInTimeStamp,
                      inBusNumber, inNumberFrames, pPlayer->pInputBuffer);

  if (!inputProcErr) {
    // inputProcErr = pPlayer->ringBuffer->Store(pPlayer->pInputBuffer,
    // inNumberFrames, pInTimeStamp->mSampleTime);
    if (pPlayer->inst.audCb) {
      pPlayer->inst.audCb(pPlayer->inst.pUserData,
                          (float *)pPlayer->pInputBuffer->mBuffers[0].mData, 1,
                          inNumberFrames);
    }
  } else {
    RIOTRACE(("AudioUnitRender returned %d.  pPlayer = 0x%x, "
              "pPlayer->pInputBuffer = 0x%x\n",
              (int)inputProcErr, (unsigned int)(uintptr_t)pPlayer,
              (unsigned int)(uintptr_t)pPlayer->pInputBuffer));
  }

  return inputProcErr;
}

// ////////////////////////////////////////////////////////////////////////////
#include "AVAudioSessionWrapper.h"
///////////////////////////////////////////////////////////////////////////////////////////////////
// We want to ensure that Unity has enabled a mode that allows recording.
///////////////////////////////////////////////////////////////////////////////////////////////////
static bool riom_set_record_category(RemoteIO_Internal_t *pPlayer) {
  const AudioSessionPropertyID id = kAudioSessionProperty_AudioCategory;
  UInt32 category = 0;
  UInt32 propSize = sizeof(category);
  LOG_ASSERT_FN(noErr == _AudioSessionGetProperty(id, &propSize, &category));

  RIOTRACE(("Category was %s\n", riomGetUintStr(category)));

  if ((category != kAudioSessionCategory_RecordAudio) &&
      (category != kAudioSessionCategory_PlayAndRecord)) {
    RIOTRACE(("Category was %s, now set to %s\n", riomGetUintStr(category),
              riomGetUintStr(kAudioSessionCategory_PlayAndRecord)));
    category = kAudioSessionCategory_PlayAndRecord;
    LOG_ASSERT_FN(noErr ==
              _AudioSessionSetProperty(id, sizeof(category), &category));
  }
  LOG_ASSERT_FN(noErr == _AudioSessionGetProperty(id, &propSize, &category));
  return ((category == kAudioSessionCategory_RecordAudio) ||
          (category == kAudioSessionCategory_PlayAndRecord));

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
*              you�re providing for play-out	    format being received
*from H/W
*
*Output Scope:	Get ASBD to inspect audio format    Set ASBD to indicate
*what format
*               being sent to H/W	                you want your units to
*receive.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////
static bool riom_set_stream_parameters(RemoteIO_Internal_t *pPlayer,
                                       const Float64 hardwareSampleRate, bool doOutput) {
  //if (!doOutput) {return true;}

  const auto kBus = (doOutput) ? kOutputBus0 : kInputBus1;
  const auto kScope = (doOutput) ? kAudioUnitScope_Input : kAudioUnitScope_Output;
  AudioStreamBasicDescription inputASBD = {0};
  
  // Get properties, and set them if necessary.
  AudioStreamBasicDescription asbdExpected = {0};
  if (doOutput){
    memset(&pPlayer->myASBD, 0, sizeof(pPlayer->myASBD));
  }
  UInt32 propSize = sizeof(pPlayer->myASBD);

  AudioStreamBasicDescription &myASBDRef = (doOutput) ? pPlayer->myASBD :inputASBD;
  
  LOG_ASSERT_FN(noErr == AudioUnitGetProperty(pPlayer->inputUnit,
                                          kAudioUnitProperty_StreamFormat,
                                              kScope, kBus,
                                          &myASBDRef, &propSize));

  if (doOutput){
    riom_print_asbd("kAudioUnitScope_Input, kOutputBus0", &myASBDRef);
  }
  else {
    riom_print_asbd("kAudioUnitScope_Output, kInputBus1", &myASBDRef);
  }

  // hardwareSampleRate = 24000.000000;
  // pPlayer->myASBD.mChannelsPerFrame = 2;
  // pPlayer->myASBD.mFormatFlags = 0x29;
  // pPlayer->myASBD.mFramesPerPacket = 1;
  // pPlayer->myASBD.mBytesPerFrame = 4;
  // pPlayer->myASBD.mBytesPerPacket = 4;
  // pPlayer->myASBD.mBitsPerChannel = 32;

  ASSERT_AT_COMPILE_TIME((kAudioFormatFlagIsNonInterleaved |
                          kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked) ==
                         0x29);
  bool propsAsExpected = true;

  asbdExpected.mChannelsPerFrame = 2;
  asbdExpected.mFormatFlags = kAudioFormatFlagIsNonInterleaved |
  kAudioFormatFlagIsFloat |
  kAudioFormatFlagIsPacked; // 0x29;
  asbdExpected.mFramesPerPacket = 1;
  asbdExpected.mBytesPerFrame = 4;
  asbdExpected.mBytesPerPacket = 4;
  asbdExpected.mBitsPerChannel = 32;

  if (!doOutput){
    asbdExpected.mChannelsPerFrame = 1;
    asbdExpected.mFormatFlags = kAudioFormatFlagIsFloat |
    kAudioFormatFlagIsPacked; // 0x29;
  }
  propsAsExpected &=
      (asbdExpected.mChannelsPerFrame == myASBDRef.mChannelsPerFrame);
  propsAsExpected &=
      (asbdExpected.mFormatFlags == myASBDRef.mFormatFlags);
  propsAsExpected &=
      (asbdExpected.mFramesPerPacket == myASBDRef.mFramesPerPacket);
  propsAsExpected &=
      (asbdExpected.mBytesPerFrame == myASBDRef.mBytesPerFrame);
  propsAsExpected &=
      (asbdExpected.mBytesPerPacket == myASBDRef.mBytesPerPacket);
  propsAsExpected &=
      (asbdExpected.mBitsPerChannel == myASBDRef.mBitsPerChannel);
  if (!propsAsExpected) {
    RIOTRACE(("Not all ASBD parameters matched expectations!, adjusting\n"));
    int err = AudioUnitSetProperty(
        pPlayer->inputUnit, kAudioUnitProperty_StreamFormat,
                                   kScope, kBus, &asbdExpected, propSize);
    if (noErr != err) {
      RIOTRACE(("AudioUnitSetProperty returned error code %d:\n", err));
      RIOTRACE(("  - Please run this software on IOS hardware.  The simulator "
                "isn't working yet.\n"));
      LOG_ASSERT(noErr == err);
    }

    myASBDRef = asbdExpected;
  }

  return propsAsExpected;
}

/**
 Using the Remote I/O audio unit (kAudioUnitSubType_RemoteIO) to perform audio processing tasks. The Remote I/O audio unit contains two buses: an input bus (microphone) and an output bus (speaker). The audio data is being processed between these buses.

 Here's a description of the connections between the different audio nodes in the code:

 Setting the audio session category: The audio session category is set to record audio using the riom_set_record_category() function. This step is necessary to allow audio input and output through the audio unit.

 Getting the audio input hardware availability: The code checks if the audio input hardware (microphone) is available using the kAudioSessionProperty_AudioInputAvailable property.

 Setting the hardware sample rate: The hardware sample rate is set using the kAudioSessionProperty_CurrentHardwareSampleRate property. If the desired sample rate is specified (pPlayer->desiredFs), it is set; otherwise, the current hardware sample rate is used.

 Setting the buffer size: The preferred buffer size is determined using the kAudioSessionProperty_PreferredHardwareIOBufferDuration property. The buffer size in frames is calculated based on the hardware duration and sample rate.

 Creating the Remote I/O audio unit: The Remote I/O audio unit is created using the AudioComponentInstanceNew() function. This audio unit handles both input (microphone) and output (speaker) audio data.

 Disabling the output hardware: The output hardware (speaker) is disabled using the kAudioOutputUnitProperty_EnableIO property and setting its value to 0 (kZeroFlag).

 Enabling the input hardware: The input hardware (microphone) is enabled using the kAudioOutputUnitProperty_EnableIO property and setting its value to 1 (kOneFlag).

 Setting audio stream parameters: The audio stream parameters are set using the riom_set_stream_parameters() function. This function configures the audio data format, channel count, and sample rate for both input and output buses.

 Setting the input callback: The input callback is set using the kAudioOutputUnitProperty_SetInputCallback property. The callback function riom_input_render_proc is responsible for processing the audio data received from the input hardware (microphone).

 The code initializes and starts the Remote I/O audio unit to begin processing audio data. When the audio data is available from the input hardware (microphone), the input callback function processes the data and stores it in the specified buffer.

 Here's a visual representation of the connections:

 ```
 +--------------+       +---------------------+       +--------------+
 | Input        |------>| Remote I/O Audio    |------>| Application  |
 | (Microphone) |       | Unit (Input Bus)    |       | (Processing) |
 +--------------+       +---------------------+       +--------------+
 ```
 In summary, the code sets up the audio session, configures the audio unit, and enables the input hardware (microphone) to process the incoming audio data. The output hardware (speaker) is disabled, and the processed audio data is stored in the application buffer.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////
static bool riom_create_input_unit(RemoteIO_Internal_t *pPlayer) {

  // Set up the RIO unit for playback
  const UInt32 kZeroFlag = 0;
  const UInt32 kOneFlag = 1;
  Float64 hardwareSampleRate = 0;
  Float32 hardwareDurationSeconds = 0;
  UInt32 bufferSizeFrames = 0;

  // Check to ensure we are using a category for recording.
  LOG_ASSERT_FN(riom_set_record_category(pPlayer));

  // Figure out if input hardware is available.
  {
    UInt32 inputAvailable = 0;
    UInt32 propSize = sizeof(inputAvailable);
    LOG_ASSERT_FN(noErr == _AudioSessionGetProperty(
                           kAudioSessionProperty_AudioInputAvailable, &propSize,
                           &inputAvailable));
    LOG_ASSERT(0 != inputAvailable);
    RIOTRACE(("inputAvailable = %u\n", (unsigned int)inputAvailable));
  }

  // Getting & setting the hardware sample rate
  // If the user wants to set a specific sample rate, then set it,
  // otherwise just get the current sample rate if it is set.
  {
    UInt32 propSize = sizeof(hardwareSampleRate);
    LOG_ASSERT_FN(noErr == _AudioSessionGetProperty(
                           kAudioSessionProperty_CurrentHardwareSampleRate,
                           &propSize, &hardwareSampleRate));
    // This used to not set hardwareSampleRate if it already was correct, but
    // on newer iPhones sometimes this gave us 44.1k even if 48k was requested.
    // Now we always request desiredFs.
    if (true){
      const int desiredFs = (pPlayer->desiredFs >= 0) ? pPlayer->desiredFs : 44100;
      RIOTRACE(("Hardware sample rate of %d not as desired.  Setting to %d\n",
                (int)hardwareSampleRate, desiredFs));
      hardwareSampleRate = desiredFs;
      LOG_ASSERT_FN(noErr == _AudioSessionSetProperty(
                             kAudioSessionProperty_CurrentHardwareSampleRate,
                             propSize, &hardwareSampleRate));

    }
    RIOTRACE(("hardwareSampleRate = %f\n", hardwareSampleRate));

    pPlayer->inst.fs = (int)hardwareSampleRate;
  }

  // Getting the preferred buffer size...
  {
    UInt32 propSize = sizeof(hardwareDurationSeconds);
    LOG_ASSERT_FN(noErr ==
              _AudioSessionGetProperty(
                  kAudioSessionProperty_PreferredHardwareIOBufferDuration,
                  &propSize, &hardwareDurationSeconds));

    if (hardwareDurationSeconds <= 0) {
      bufferSizeFrames = DEFAULT_BUFFER_SIZE_FRAMES;
      hardwareDurationSeconds = bufferSizeFrames / hardwareSampleRate;
      RIOTRACE(("Invalid hardwareDurationSeconds, setting to = %f ms\n",
                1000.0f * hardwareDurationSeconds));
    } else {
      RIOTRACE(("hardwareDurationSeconds = %f ms\n",
                1000.0f * hardwareDurationSeconds));
      Float32 fSamps = hardwareDurationSeconds * hardwareSampleRate;
      RIOTRACE(("approximate buffer frames = %f\n", fSamps));
      Float32 fSampsLog2 = logf(fSamps) / logf(2);
      fSampsLog2 = ceil(fSampsLog2);
      bufferSizeFrames = powf(2, fSampsLog2);
      RIOTRACE(("actual buffer frames = %d\n", (int)bufferSizeFrames));
    }
  }

  // Getting RemoteIO AudioUnit from Audio Component Manager
  {
    AudioComponentDescription audioCompDesc = {0};

    audioCompDesc.componentType = kAudioUnitType_Output;
    audioCompDesc.componentSubType = kAudioUnitSubType_RemoteIO;
    audioCompDesc.componentManufacturer = kAudioUnitManufacturer_Apple;

    // Get the RIO unit from the audio component manager
    RIOTRACE(("Getting input component...\n"));
    AudioComponent rioComponent = AudioComponentFindNext(NULL, &audioCompDesc);
    LOG_ASSERT_FN(noErr ==
              AudioComponentInstanceNew(rioComponent, &pPlayer->inputUnit));
    RIOTRACE(("Got component!"));
  }

  // Stop the hardware so we can initialize some stuff.
  LOG_ASSERT_FN(noErr == AudioOutputUnitStop(pPlayer->inputUnit));

  // Set the stream parameters!
  LOG_ASSERT_FN(riom_set_stream_parameters(pPlayer, hardwareSampleRate, true));

  // Disable output hardware. This disables the output of kOutputBus0
  LOG_ASSERT_FN(noErr == AudioUnitSetProperty(pPlayer->inputUnit,
                                          kAudioOutputUnitProperty_EnableIO,
                                          kAudioUnitScope_Output, kOutputBus0,
                                          &kZeroFlag, sizeof(kZeroFlag)));

  // Enable microphone hardware if not already enabled!
  {
    UInt32 micHwEnabled = 0;
    UInt32 flagSize = sizeof(micHwEnabled);

    // Gets the input of kInputBus1
    LOG_ASSERT_FN(noErr == AudioUnitGetProperty(pPlayer->inputUnit,
                                            kAudioOutputUnitProperty_EnableIO,
                                            kAudioUnitScope_Input, kInputBus1,
                                            &micHwEnabled, &flagSize));

    RIOTRACE(("micHwEnabled = %u\n", (unsigned int)micHwEnabled));

    // If disabled, then enable it.
    if (micHwEnabled == 0) {
           
      RIOTRACE(("Enabling microphone hardware.\n"));
      
      //LOG_ASSERT_FN(riom_set_stream_parameters(pPlayer, hardwareSampleRate, false));
      
      // Enable the input of kInputBus1
      LOG_ASSERT_FN(noErr == AudioUnitSetProperty(pPlayer->inputUnit,
                                              kAudioOutputUnitProperty_EnableIO,
                                              kAudioUnitScope_Input, kInputBus1,
                                              &kOneFlag, sizeof(kOneFlag)));
      
      // Now get status - get the input flag of inputBus1
      LOG_ASSERT_FN(noErr == AudioUnitGetProperty(pPlayer->inputUnit,
                                              kAudioOutputUnitProperty_EnableIO,
                                              kAudioUnitScope_Input, kInputBus1,
                                              &micHwEnabled, &flagSize));

      RIOTRACE(("micHwEnabled = %u\n", (unsigned int)micHwEnabled));
    }
  }

  // Do some logging so we know the settings used.
  {
    AudioStreamBasicDescription asbd = {0};
    UInt32 propSize = sizeof(asbd);

    LOG_ASSERT_FN(noErr == AudioUnitGetProperty(pPlayer->inputUnit,
                                            kAudioUnitProperty_StreamFormat,
                                            kAudioUnitScope_Input, kOutputBus0,
                                            &asbd, &propSize));

    riom_print_asbd("kAudioUnitScope_Input,kOutputBus0(spkr)", &asbd);

    LOG_ASSERT_FN(noErr == AudioUnitGetProperty(pPlayer->inputUnit,
                                            kAudioUnitProperty_StreamFormat,
                                            kAudioUnitScope_Output, kInputBus1,
                                            &asbd, &propSize));

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
    UInt32 bufferSizeBytes = bufferSizeFrames * pPlayer->myASBD.mBytesPerFrame;
    const int numBuffers = pPlayer->myASBD.mChannelsPerFrame;
    LOG_ASSERT(numBuffers >= 1);

    // This code will ONLY work if non-interleaved is set, so there are
    // independent buffers for L and R.
    LOG_ASSERT((1 == numBuffers) ||
           (pPlayer->myASBD.mFormatFlags & kAudioFormatFlagIsNonInterleaved));

    // malloc buffer lists. use numBuffers -1 because AudioBuffer mBuffers[1] is a variable length array.
    pPlayer->pInputBuffer = (AudioBufferList *)malloc(
        sizeof(AudioBufferList) + ((numBuffers - 1) * sizeof(AudioBuffer)));

    RIOTRACE(("pPlayer->myASBD.mChannelsPerFrame = %u\n",
              (unsigned int)pPlayer->myASBD.mChannelsPerFrame));
    pPlayer->pInputBuffer->mNumberBuffers = numBuffers;

    // Pre-malloc buffers for AudioBufferLists
    for (UInt32 i = 0; i < pPlayer->pInputBuffer->mNumberBuffers; i++) {
      pPlayer->pInputBuffer->mBuffers[i].mNumberChannels = 1;
      pPlayer->pInputBuffer->mBuffers[i].mDataByteSize = bufferSizeBytes;
      pPlayer->pInputBuffer->mBuffers[i].mData = malloc(bufferSizeBytes);
    }
  }

  // Set a new callback struct, but save the old one first.
  {
    UInt32 propSize = sizeof(pPlayer->oldRenderCallbackStruct);
    memset(&pPlayer->oldRenderCallbackStruct, 0,
           sizeof(pPlayer->oldRenderCallbackStruct));

    LOG_ASSERT_FN(noErr == AudioUnitGetProperty(
                           pPlayer->inputUnit,
                           kAudioOutputUnitProperty_SetInputCallback,
                           kAudioUnitScope_Global, 0,
                           &pPlayer->oldRenderCallbackStruct, &propSize));

    RIOTRACE(
        ("Saving the old callback = 0x%x\n",
         (unsigned int)(uintptr_t)pPlayer->oldRenderCallbackStruct.inputProc));

    // Set the callback method to point to our new render method.
    AURenderCallbackStruct callbackStruct;
    callbackStruct.inputProc = riom_input_render_proc;
    callbackStruct.inputProcRefCon = pPlayer;
    LOG_ASSERT_FN(noErr ==
              AudioUnitSetProperty(pPlayer->inputUnit,
                                   kAudioOutputUnitProperty_SetInputCallback,
                                   kAudioUnitScope_Global, 0, &callbackStruct,
                                   sizeof(callbackStruct)));
  }

  // Initialize and start the RIO unit
  LOG_ASSERT_FN(noErr == AudioUnitInitialize(pPlayer->inputUnit));
  LOG_ASSERT_FN(noErr == AudioOutputUnitStart(pPlayer->inputUnit));

  RIOTRACE(("RIO started!\n"));

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Starts the microphone, which will start triggering callbacks.
///////////////////////////////////////////////////////////////////////////////////////////////////
RioInstance_t *rio_start_mic(
  void *const pSelf,
  void *const pUserData,
  fnAudioCallbackFn fnPtr,
  const int desiredFs) {

  RemoteIO_Internal_t *pInst =
      (RemoteIO_Internal_t *)malloc(sizeof(RemoteIO_Internal_t));

  assert(nullptr != pInst);
  
  memset(pInst, 0, sizeof(RemoteIO_Internal_t));
  pInst->inst.pUserData = pUserData;
  pInst->inst.audCb = fnPtr;
  pInst->desiredFs = desiredFs;

  riom_create_input_unit(pInst);

#if 0 // TARGET_IOS_IPHONE
  CheckError(AudioSessionSetActive(true), "Couldn't re-set audio session active");
#endif
  return &pInst->inst;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
static bool rio_restore_old_callback(RemoteIO_Internal_t *pPlayer) {

  LOG_ASSERT_FN(noErr ==
            AudioUnitSetProperty(
                pPlayer->inputUnit, kAudioOutputUnitProperty_SetInputCallback,
                kAudioUnitScope_Global, 0, &pPlayer->oldRenderCallbackStruct,
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
