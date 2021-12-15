#MAKE?=mingw32-make
AR?=ar
ARFLAGS?=rcs
PATHSEP?=/
CC=gcc
BUILDROOT?=build

ifeq ($(CLANG),1)
	export CC=clang
endif

BUILDDIR?=$(BUILDROOT)$(PATHSEP)$(CC)
BUILDPATH?=$(BUILDDIR)$(PATHSEP)

INSTALL_ROOT?=$(BUILDPATH)

#DEBUGSYMBOLS=-ggdb
DEBUGSYMBOLS=-g

ifeq ($(DEBUG),1)
	export debug=$(DEBUGSYMBOLS) -Ddebug=1
	export isdebug=1
endif

ifeq ($(ANALYSIS),1)
	export analysis=-Danalysis=1
	export isanalysis=1
endif

ifeq ($(DEBUG),2)
	export debug=$(DEBUGSYMBOLS) -Ddebug=2
	export isdebug=1
endif

ifeq ($(DEBUG),3)
	export debug=$(DEBUGSYMBOLS) -Ddebug=3
	export isdebug=1
endif

ifeq ($(OUTPUT),1)
	export outimg= -Doutput=1
endif


BIT_SUFFIX=

ifeq ($(M32),1)
	CFLAGS+=-m32
	BIT_SUFFIX+=32
endif

CFLAGS+= -std=c11 -Wpedantic -pedantic-errors -Wall -Wextra $(debug)
#-ggdb
#-pg for profiling 

LIBDIR?=/c/dev/lib$(BIT_SUFFIX)
LIBSDIR?=-L$(LIBDIR)
INCLUDE?=-I/c/dev/include/freetype2 -I/c/dev/include -I/usr/include -I/usr/include/freetype2 -I./src

INCLUDEDIR=$(INCLUDE)

_SRC_FILES=rft_converter_main rft_conv_param_builder
SRC=$(patsubst %,src/%,$(patsubst %,%.c,$(_SRC_FILES)))
#$(info $$_TESTSRC is [${_TESTSRC}])
BIN=rft_converter.exe

LIBSDIR+=-L./$(BUILDDIR)

_LIB_SRC_FILES=rft_converter
LIBSRC=$(patsubst %,src/%,$(patsubst %,%.c,$(_LIB_SRC_FILES)))
OBJS=rft_converter.o
LIB_NAME=rft_converter
LIBS= $(LIB_NAME) freetype

TESTLIB=$(patsubst %,-l%,$(LIBS))
_TEST_SRC_FILES=test_rft_conv_param_builder rft_conv_param_builder
TESTSRC=$(patsubst %,src/%,$(patsubst %,%.c,$(_TEST_SRC_FILES)))
TESTBIN=test_rft_conv_param_builder.exe

LIB=lib$(LIB_NAME).a

all: mkbuilddir $(BIN)

$(BUILDPATH)$(OBJS):
	$(CC) $(CFLAGS) -c $(LIBSRC) $(INCLUDEDIR) -o $(BUILDPATH)$(OBJS)

$(BUILDPATH)$(LIB): $(BUILDPATH)$(OBJS)
	$(AR) $(ARFLAGS) $(BUILDPATH)$(LIB) $(BUILDPATH)$(OBJS)

.PHONY: clean mkbuilddir test

test: $(BUILDPATH)$(TESTBIN)
	./$(BUILDPATH)$(TESTBIN)

mkbuilddir:
	mkdir -p $(BUILDDIR)

clean:
	-rm -dr $(BUILDROOT) $(TESTBIN)

$(BIN): src/rft_converter_main.c $(BUILDPATH)$(OBJS) src/rft_converter.c src/rft_converter.h src/rft_conv_param_builder.c src/rft_conv_param_builder.h
	$(CC) $(CFLAGS) $(SRC) $(BUILDPATH)$(OBJS) $(LIBSDIR) -lfreetype $(INCLUDE) $(debug) -o $(BIN)

$(BUILDPATH)$(TESTBIN): src/test_rft_conv_param_builder.c src/rft_conv_param_builder.c src/rft_conv_param_builder.h
	$(CC) $(CFLAGS) $(TESTSRC) $(LIBSDIR) -lfreetype $(INCLUDE) $(debug) -o $(BUILDPATH)$(TESTBIN)
