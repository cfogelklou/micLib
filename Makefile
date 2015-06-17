CC = clang
CXX = clang++
LD =  clang++
LDLIB = libtool

FLAGS = -O3 -m64 -g

MYINCLUDES = \
-Ilib_source \
-I../lib_source \
-Ic_sources \
-I../c_sources \
-I/System/Library/Frameworks

CFLAGS += $(FLAGS) $(MYINCLUDES)
CPPFLAGS += $(FLAGS) $(MYINCLUDES)
LDFLAGS = -static 
LDAPPFLAGS = $(FLAGS)
LDAPPFLAGS += -framework AudioToolbox -framework AudioUnit -framework CoreAudio

SRCDIR = lib_source
OUTDIR = _osx

CSRCS = $(wildcard $(SRCDIR)/*.c)
# CSRCS += $(wildcard $(SRCDIR_APPLE)/*.c)
COBJS = $(CSRCS:%.c=$(OUTDIR)/%.o)

CPPSRCS = $(wildcard $(SRCDIR)/*.cpp)
CPPOBJS = $(CPPSRCS:%.cpp=$(OUTDIR)/%.o)

TESTSRCS = remoteio_mic_test.cpp
TESTOBJS = $(CPPOBJS) $(TESTSRCS:%.cpp=$(OUTDIR)/%.o)

TARGETLIB = $(OUTDIR)/libremote_io.a

TARGET = $(OUTDIR)/remoteio_mic_test

all: $(OUTDIR) $(TARGETLIB) $(TARGET)
	@echo "=== done $(LDFLAGS)"

$(OUTDIR) :
	@mkdir -p $@/$(SRCDIR)

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

