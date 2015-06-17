CC = clang
CXX = clang++
LD =  clang++
LDLIB = libtool

FLAGS = -O3 -m64 -g

MYINCLUDES = \
-Iapple_source \
-I../apple_source \
-Icommon \
-I../common \
-I/System/Library/Frameworks

CFLAGS += $(FLAGS) $(MYINCLUDES)
CPPFLAGS += $(FLAGS) $(MYINCLUDES)
LDFLAGS = -static 
LDAPPFLAGS = $(FLAGS)
LDAPPFLAGS += -framework AudioToolbox -framework AudioUnit -framework CoreAudio

SRCDIR = apple_source
SRCDIR2 = common
OUTDIR = _osx

CSRCS = $(wildcard $(SRCDIR)/*.c)
CSRCS += $(wildcard $(SRCDIR2)/*.c)
COBJS = $(CSRCS:%.c=$(OUTDIR)/%.o)

CPPSRCS = $(wildcard $(SRCDIR)/*.cpp)
CPPSRCS += $(wildcard $(SRCDIR2)/*.cpp)
CPPOBJS = $(CPPSRCS:%.cpp=$(OUTDIR)/%.o)

TESTSRCS = remoteio_mic_test.cpp
TESTOBJS = $(CPPOBJS) $(TESTSRCS:%.cpp=$(OUTDIR)/%.o)

TARGETLIB = $(OUTDIR)/libremote_io.a

TARGET = $(OUTDIR)/remoteio_mic_test

all: $(OUTDIR) $(TARGETLIB) $(TARGET)
	@echo "=== done $(LDFLAGS)"

$(OUTDIR) :
	@mkdir -p $@/$(SRCDIR)
	@mkdir -p $@/$(SRCDIR2)

$(OUTDIR)/%.o : %.c
	@echo "  CC $@"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(OUTDIR)/%.o : %.cpp
	@echo "  CXX $@"
	@$(CXX) $(CPPFLAGS)  -c -o $@ $<

$(TARGETLIB) : $(COBJS) $(CPPOBJS)
	@echo "  LD $@"
	@$(LDLIB) $(LDFLAGS) -o $@ $^

$(TARGET) : $(COBJS) $(TESTOBJS)
	@echo "  LD $@"
	@$(CXX) $(LDAPPFLAGS)  -o $@ $^

clean:
	@echo "=== removing $(OUTDIR)"
	@rm -rf $(OUTDIR)

