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
LIBSDIR?=-L$(LIBDIR) -L./$(BUILDPATH)
INCLUDE?=-I/c/dev/include/freetype2 -I/c/dev/include -I/usr/include -I/usr/include/freetype2 -I./src

_SRC_FILES=rft_converter_main rft_conv_param_builder
SRC=$(patsubst %,src/%,$(patsubst %,%.c,$(_SRC_FILES)))

BIN=$(BUILDPATH)$(NAME)_main.exe

NAME=rft_converter

OBJS=$(BUILDPATH)rft_converter_main.o $(BUILDPATH)rft_converter.o $(BUILDPATH)rft_conv_param_builder.o

LIBNAME=lib$(NAME).a
LIB=$(BUILDPATH)$(LIBNAME)

TESTLIB=$(patsubst %,-l%, $(NAME) freetype geometry utilsmath mat vec dl_list )
TESTSRC=$(patsubst %,src/%,$(patsubst %,%.c, test_rft_conv_param_builder rft_conv_param_builder ))
TESTBIN=$(BUILDPATH)test_rft_conv_param_builder.exe

all: mkbuilddir $(LIB) $(BIN) $(TESTBIN)

$(OBJS): src/rft_converter.c
	$(CC) $(CFLAGS) -c src/$(@F:.o=.c) -o $@ $(INCLUDE)

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(BIN): $(LIB)
	$(CC) $(CFLAGS) src/$(@F:.exe=.c) -o $@ $(LIBSDIR) $(TESTLIB) $(INCLUDE) $(debug)
	mv $@ $(BUILDPATH)$(NAME).exe

$(TESTBIN): src/test_rft_conv_param_builder.c
	$(CC) $(CFLAGS) src/$(@F:.exe=.c) -o $@ $(LIBSDIR) $(TESTLIB) $(INCLUDE) $(debug)

.PHONY: clean mkbuilddir test

test: $(TESTBIN)
	./$(TESTBIN)

mkbuilddir:
	mkdir -p $(BUILDDIR)

clean:
	-rm -dr $(BUILDROOT)

