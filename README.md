# micLib
A simple, low latency library for accessing the microphone in IOS (and possibly, eventually, Android).

Note: build_ios_fat_lib.sh probably no longer works.  Instead, use the xcode library project, which does work, to build libremote_io.a.
Then try the library on real hardware by running the xcode project.  As of right now, does not run on a simulator, unfortunately.

## How to Integrate with Unity ##
Copy the following files to your Assets/Plugins/iOS directory:
- libremote_io.a
- apple_source/remoteio_mic_c.h
- common/audiolib_types.h
- common/PcmQ.h
- micWrapper.cpp

## Unity Capture ##
You can then use the example in unity/MicCapture.cs as a template for your microphone handling code.
