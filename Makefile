_CFLAGS:=$(CFLAGS)
CFLAGS:=$(_CFLAGS)
_LDFLAGS:=$(LDFLAGS)
LDFLAGS:=$(_LDFLAGS)

ARFLAGS?=rcs
PATHSEP?=/
BUILDROOT?=build

BUILDDIR?=$(BUILDROOT)$(PATHSEP)$(CC)
BUILDPATH?=$(BUILDDIR)$(PATHSEP)

ifndef PREFIX
	INSTALL_ROOT=$(BUILDPATH)
else
	INSTALL_ROOT=$(PREFIX)$(PATHSEP)
	ifeq ($(INSTALL_ROOT),/)
	INSTALL_ROOT=$(BUILDPATH)
	endif
endif

ifdef DEBUG
	CFLAGS+=-ggdb
	ifeq ($(DEBUG),)
	CFLAGS+=-Ddebug=1
	else 
	CFLAGS+=-Ddebug=$(DEBUG)
	endif
endif

ifeq ($(M32),1)
	CFLAGS+=-m32
	BIT_SUFFIX+=32
endif

CFLAGS+=-std=c11 -Wpedantic -pedantic-errors -Wall -Wextra
#-ggdb
#-pg for profiling 

LDFLAGS+=-L/c/dev/lib$(BIT_SUFFIX) -L./$(BUILDPATH)
CFLAGS+=-I./src -I/c/dev/include/freetype2 -I/c/dev/include -I/usr/include -I/usr/include/freetype2

_SRC_FILES=rft_converter_main rft_conv_param_builder
SRC=$(patsubst %,src/%,$(patsubst %,%.c,$(_SRC_FILES)))

BIN=$(BUILDPATH)$(NAME)_main.exe

NAME=rft_converter

OBJS=$(BUILDPATH)rft_converter_main.o $(BUILDPATH)rft_converter.o $(BUILDPATH)rft_conv_param_builder.o

LIBNAME=lib$(NAME).a
LIB=$(BUILDPATH)$(LIBNAME)

LDFLAGS+=$(patsubst %,-l%, $(NAME) freetype geometry utilsmath mat vec dl_list )
TESTSRC=$(patsubst %,src/%,$(patsubst %,%.c, test_rft_conv_param_builder rft_conv_param_builder ))
TESTBIN=$(BUILDPATH)test_rft_conv_param_builder.exe

all: mkbuilddir $(LIB) $(BIN) 

$(OBJS): src/rft_converter.c
	$(CC) $(CFLAGS) -c src/$(@F:.o=.c) -o $@

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(BIN): $(LIB)
	$(CC) $(CFLAGS) src/$(@F:.exe=.c) -o $@ $(LDFLAGS)
	mv $@ $(BUILDPATH)$(NAME).exe

$(TESTBIN): $(LIB) src/test_rft_conv_param_builder.c
	$(CC) $(CFLAGS) src/$(@F:.exe=.c) -o $@ $(LDFLAGS)

.PHONY: clean mkbuilddir test

test: mkbuilddir $(TESTBIN)
	./$(TESTBIN)

mkbuilddir:
	mkdir -p $(BUILDDIR)

clean:
	-rm -dr $(BUILDROOT)

