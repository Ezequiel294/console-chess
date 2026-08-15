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

# VERSION is the project's single declared version (see openspec change
# project-versioning): every other place a version appears derives from it.
# A missing or malformed file must stop the build before anything is
# compiled, so the failure names the version problem instead of surfacing
# as a strange compiler error further down.
#
# SAVE_VERSION_FIELD_LEN matches the fixed-size version field in
# src/app/save.c; a version that would not fit, including its terminating
# NUL, is a build-time failure rather than a runtime truncation.
SAVE_VERSION_FIELD_LEN := 16

ifeq ($(wildcard VERSION),)
$(error VERSION file not found at the repository root; create one containing a version such as "1.1.0")
endif

VERSION := $(strip $(shell cat VERSION))

ifeq ($(VERSION),)
$(error VERSION file is empty or contains only whitespace)
endif

ifeq ($(shell echo '$(VERSION)' | grep -Eq '^[0-9]+\.[0-9]+\.[0-9]+(-[0-9A-Za-z][0-9A-Za-z.-]*)?$$' && echo ok),)
$(error VERSION file contains an invalid version string: "$(VERSION)" (expected MAJOR.MINOR.PATCH with an optional pre-release suffix))
endif

VERSION_LEN := $(shell printf '%s' '$(VERSION)' | wc -c)
ifeq ($(shell test $(VERSION_LEN) -lt $(SAVE_VERSION_FIELD_LEN) && echo ok),)
$(error VERSION "$(VERSION)" is too long for the save file's $(SAVE_VERSION_FIELD_LEN)-byte version field)
endif

# Quoted so it arrives at the compiler as a C string literal.
CPPFLAGS := -DCHESS_VERSION='"$(VERSION)"'

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
	$(CC) $(STD) $(WARN) $(INCLUDE) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

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
