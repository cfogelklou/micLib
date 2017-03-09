# micLib
A simple, low latency library for accessing the microphone in IOS (and possibly, eventually, Android).

Start by running build_ios_fat_lib.sh
Then try the library on real hardware by running the xcode project.  As of right now, does not run on a simulator, unfortunately.

## How to Integrate with Unity ##
Copy the following files to your Assets/Plugins/iOS directory:
- libremote_io.a
- apple_source/remoteio_mic_c.h
- common/audiolib_types.h
- common/PcmQ.h
- micWrapper.cpp
