rm libremote_io.a
make -f Makefile_ios.mak clean
make -f Makefile_ios.mak ARCH=armv6
make -f Makefile_ios.mak ARCH=x86_64 SIM=1
make -f Makefile_ios.mak ARCH=armv7
make -f Makefile_ios.mak ARCH=armv7s
make -f Makefile_ios.mak ARCH=arm64
lipo -create _ios/armv7/libremote_io.a _ios/armv7s/libremote_io.a _ios/arm64/libremote_io.a _ios/x86_64/libremote_io.a _ios/armv6/libremote_io.a -output libremote_io.a
ln -s ../../Base\ App/band-buddy/Assets/Plugins/iOS/ plugins_ios
cp libremote_io.a plugins_ios/
cp libremote_io.a remoteio_test/

