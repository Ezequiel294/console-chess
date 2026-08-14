# Console Chess
#
# make          build ./console-chess
# make run      build and play
# make debug    build ./console-chess-debug with sanitizers and no optimisation
# make clean    remove all build output

CC       ?= cc
CFLAGS   ?= -O2
LDFLAGS  ?=

# Not overridable: these define the language and the warning contract.
STD      := -std=c17
WARN     := -Wall -Wextra

# -MMD -MP makes the compiler emit a .d file per object listing the headers it
# read, so editing a header rebuilds every .c that includes it.
DEPFLAGS := -MMD -MP

# Headers are included by their path below src/, e.g. #include "core/board.h",
# so one include directory covers the whole tree and every include says which
# layer it reaches into.
SRCDIR   := src
INCLUDE  := -I$(SRCDIR)

BIN      := console-chess
BUILDDIR := build/release

SRC      := $(wildcard $(SRCDIR)/*.c $(SRCDIR)/*/*.c)
OBJ      := $(SRC:%.c=$(BUILDDIR)/%.o)
DEP      := $(OBJ:.o=.d)

.PHONY: all run debug clean

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILDDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(STD) $(WARN) $(INCLUDE) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

run: all
	./$(BIN)

# Separate binary and object directory so a debug build never leaves
# sanitiser-instrumented objects behind for the release build to link.
debug:
	$(MAKE) BIN=console-chess-debug BUILDDIR=build/debug \
	        CFLAGS='-O0 -g3 -fno-omit-frame-pointer -fsanitize=address,undefined' \
	        LDFLAGS='-fsanitize=address,undefined' \
	        all

clean:
	rm -rf build console-chess console-chess-debug

-include $(DEP)
