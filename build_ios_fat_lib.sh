#!/bin/sh
rm _ios/libremote_io.a
make -f Makefile_ios.mak clean
#make -f Makefile_ios.mak ARCH=armv6
make -f Makefile_ios.mak ARCH=x86_64 SIM=1 ROBOVM=1
make -f Makefile_ios.mak ARCH=armv7 ROBOVM=1
make -f Makefile_ios.mak ARCH=armv7s ROBOVM=1
make -f Makefile_ios.mak ARCH=arm64 ROBOVM=1
lipo -create _ios/armv7/libremote_io.a _ios/armv7s/libremote_io.a _ios/arm64/libremote_io.a _ios/x86_64/libremote_io.a -output _ios/libremote_io.a
cp _ios/libremote_io.a remoteio_test/

