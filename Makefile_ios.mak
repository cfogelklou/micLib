# Note: Build this with make -f Makefile_ios.mak
# Tested on AppSupport Macbook Pro.
# Include paths can probably be made... better.

export LANG=en_US.US-ASCII

export IPHONEOS_DEPLOYMENT_TARGET=6.1

CC = clang
CXX = clang++
LD = clang++
LDLIB = libtool

ifdef SIM

ARCH=x86_64
PLATFORMPATH=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform
export PATH="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/usr/bin:/Applications/Xcode.app/Contents/Developer/usr/bin:/usr/bin:/bin:/usr/sbin:/sbin"
SDKPATH = $(PLATFORMPATH)/Developer/SDKs/iPhoneSimulator.sdk

PFLAGS=-mios-simulator-version-min=6.1

else

PLATFORMPATH=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform
export PATH="/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin:/Applications/Xcode.app/Contents/Developer/usr/bin:/usr/bin:/bin:/usr/sbin:/sbin"
SDKPATH = $(PLATFORMPATH)/Developer/SDKs/iPhoneOS.sdk

PFLAGS=-miphoneos-version-min=6.1

endif

#armv6
#armv7
#armv7s
#arm64
ifndef ARCH
ARCH=arm64
endif

MYINCLUDES = \
-Iapple_source \
-I../apple_source \
-Icommon \
-I../common \
-I$(PLATFORMPATH)/Developer/Library/Frameworks \
-I$(SDKPATH) \
-I$(SDKPATH)/usr/include \
-I/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include \
-I/System/Library/Frameworks/JavaVM.framework/Versions/A/Headers \
-I/System/Library/Frameworks/JavaVM.framework/Versions/Current/Headers

FLAGS = -DTARGET_OS_IPHONE=1 $(MYINCLUDES)

CFLAGS = $(PFLAGS) -x c -arch $(ARCH) $(FLAGS) -fmessage-length=0 -fdiagnostics-show-note-include-stack -fmacro-backtrace-limit=0 -std=gnu99 -Wnon-modular-include-in-framework-module -Werror=non-modular-include-in-framework-module -Wno-trigraphs -fpascal-strings -Os -Wno-missing-field-initializers -Wno-missing-prototypes -Werror=return-type -Wunreachable-code -Werror=deprecated-objc-isa-usage -Werror=objc-root-class -Wno-missing-braces -Wparentheses -Wswitch -Wunused-function -Wno-unused-label -Wno-unused-parameter -Wunused-variable -Wunused-value -Wempty-body -Wconditional-uninitialized -Wno-unknown-pragmas -Wno-shadow -Wno-four-char-constants -Wno-conversion -Wconstant-conversion -Wint-conversion -Wbool-conversion -Wenum-conversion -Wshorten-64-to-32 -Wpointer-sign -Wno-newline-eof -isysroot $(SDKPATH) -fstrict-aliasing -Wdeprecated-declarations -g -Wno-sign-conversion -MMD -MT dependencies
  
CPPFLAGS = $(PFLAGS) -x c++ -arch $(ARCH) $(FLAGS) -fmessage-length=0 -fdiagnostics-show-note-include-stack -fmacro-backtrace-limit=0 -std=gnu++11 -stdlib=libc++ -fmodules -Wnon-modular-include-in-framework-module -Werror=non-modular-include-in-framework-module -Wno-trigraphs -fpascal-strings -Os -Wno-missing-field-initializers -Wno-missing-prototypes -Werror=return-type -Wunreachable-code -Werror=deprecated-objc-isa-usage -Werror=objc-root-class -Wno-non-virtual-dtor -Wno-overloaded-virtual -Wno-exit-time-destructors -Wno-missing-braces -Wparentheses -Wswitch -Wunused-function -Wno-unused-label -Wno-unused-parameter -Wunused-variable -Wunused-value -Wempty-body -Wconditional-uninitialized -Wno-unknown-pragmas -Wno-shadow -Wno-four-char-constants -Wno-conversion -Wconstant-conversion -Wint-conversion -Wbool-conversion -Wenum-conversion -Wshorten-64-to-32 -Wno-newline-eof -Wno-c++11-extensions -isysroot $(SDKPATH) -fstrict-aliasing -Wdeprecated-declarations -Winvalid-offsetof -g -Wno-sign-conversion -MMD -MT dependencies

LDFLAGS =  -static -arch_only $(ARCH) -syslibroot $(SDKPATH)

LDAPPFLAGS = $(FLAGS)
LDAPPFLAGS += \
  -F$(SDKPATH)/System/Library/Frameworks \
  -framework AudioToolbox -framework AudioUnit -framework CoreAudio

SRCDIR = apple_source
SRCDIR2 = common
OUTDIR_TOP = _ios
OUTDIR = $(OUTDIR_TOP)/$(ARCH)

CSRCS = $(wildcard $(SRCDIR)/*.c)
CSRCS += $(wildcard $(SRCDIR2)/*.c)

COBJS = $(CSRCS:%.c=$(OUTDIR)/%.o)

CPPSRCS = $(wildcard $(SRCDIR)/*.cpp)
CPPSRCS += $(wildcard $(SRCDIR2)/*.cpp)
CPPOBJS = $(CPPSRCS:%.cpp=$(OUTDIR)/%.o)

TESTSRCS = main.cpp
TESTOBJS = $(CPPOBJS) $(TESTSRCS:%.cpp=$(OUTDIR)/%.o)

TARGETLIB = $(OUTDIR)/libremote_io.a

#TARGET = $(OUTDIR)/main

all: $(OUTDIR) $(TARGETLIB)
	@echo "=== done $(LDFLAGS)"

$(OUTDIR) :
	@mkdir -p $@/$(SRCDIR2)
	@mkdir -p $@/$(SRCDIR)

$(OUTDIR)/%.o : %.c
	@echo "  CC $@"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(OUTDIR)/%.o : %.cpp
	@echo "  CXX $@"
	@$(CXX) $(CPPFLAGS)  -c -o $@ $<

$(TARGETLIB) : $(COBJS) $(CPPOBJS)
	@echo "  LD $@"
	@$(LDLIB) $(LDFLAGS)  -o $@ $^

$(TARGET) : $(COBJS) $(TESTOBJS)
	@echo "  LD $@"
	@$(CXX) $(LDAPPFLAGS)  -o $@ $^

clean:
	@echo "=== removing $(OUTDIR_TOP)"
	@rm -rf $(OUTDIR_TOP)
	
