# Copyright (c) 2021 Gennady Trafimenkov
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
# OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.


ifneq "$(shell uname)" "Linux"
  $(error This build script must run on Linux)
endif

# By default building binary for whatever system the build system is running
TARGET_ARCH    ?=

# Place for object and binary files
RELEASE_DIR    := build
ifeq "$(TARGET_ARCH)" ""
BUILD_DIR      := build/default
else
BUILD_DIR      := build/$(TARGET_ARCH)
endif

# The main binary will be built from content of src folder plus the static library
ifeq "$(TARGET_ARCH)" ""
MAIN_BINARY  := ja2-vanilla-cp
else
MAIN_BINARY  := ja2-vanilla-cp-$(TARGET_ARCH)
endif

MAIN_SOURCES := $(shell find Build -name '*.cc') $(shell find sgp -name '*.cc')
MAIN_SOURCES += $(shell find _build/lib-slog/slog -name '*.c')
MAIN_SOURCES += _build/lib-smacker/libsmacker/smacker.c
MAIN_SOURCES += _build/lib-smacker/libsmacker/smk_hufftree.c
MAIN_SOURCES += _build/lib-smacker/libsmacker/smk_bitstream.c
MAIN_SOURCES += _build/lib-gtest/src/gtest-all.cc
MAIN_OBJS0   := $(filter %.o, $(MAIN_SOURCES:.c=.o) $(MAIN_SOURCES:.cc=.o) $(MAIN_SOURCES:.cpp=.o))
MAIN_OBJS    := $(addprefix $(BUILD_DIR)/,$(MAIN_OBJS0))
MAIN_DEPS    := $(OBJS:.o=.d)

ifeq "$(TARGET_ARCH)" "linux-gcc-amd64"
	AR             := ar
	CC             := gcc
	CXX            := g++
	CFLAGS         += -m64
endif

ifeq "$(TARGET_ARCH)" "linux-gcc-x86"
	AR             := ar
	CC             := gcc
	CXX            := g++
	CFLAGS         += -m32
endif

ifeq "$(TARGET_ARCH)" "linux-clang-amd64"
	AR             := ar
	CC             := clang
	CXX            := clang++
	CFLAGS         += -m64
endif

ifeq "$(TARGET_ARCH)" "linux-clang-x86"
	AR             := ar
	CC             := clang
	CXX            := clang++
	CFLAGS         += -m32
endif

ifeq "$(TARGET_ARCH)" "win32"
	AR             := i686-w64-mingw32-ar
	CC             := i686-w64-mingw32-gcc
	CXX            := i686-w64-mingw32-g++
	LOCAL_SDL_LIB  := _build/lib-SDL2-mingw/i686-w64-mingw32
endif

ifeq "$(TARGET_ARCH)" "win64"
	AR             := x86_64-w64-mingw32-ar
	CC             := x86_64-w64-mingw32-gcc
	CXX            := x86_64-w64-mingw32-g++
endif

ifdef DEBUGBUILD
CFLAGS += -g
endif

CFLAGS += -I .
CFLAGS += -I Build
CFLAGS += -I _build/lib-slog
CFLAGS += -I _build/lib-smacker/libsmacker
CFLAGS += -I _build/lib-utf8cpp/source
CFLAGS += -I _build/lib-gtest/include
CFLAGS += -I _build/lib-gtest

ifdef LOCAL_SDL_LIB
	CFLAGS_SDL  := $(shell _build/lib-SDL2-mingw/i686-w64-mingw32/bin/sdl2-config --cflags | sed s@/usr/local/i686-w64-mingw32/@$(LOCAL_SDL_LIB)/@g)
	# if you want to leave the console window (too see log), remove flag "-mwindows" from LDFLAGS_SDL
	LDFLAGS_SDL := $(shell _build/lib-SDL2-mingw/i686-w64-mingw32/bin/sdl2-config --static-libs | sed s@/usr/local/i686-w64-mingw32/@$(LOCAL_SDL_LIB)/@g)
	# completely static build, SDL2.dll will not be required
	LDFLAGS     += -static
	# SDL2.dll required, libstdc++6.dll is not
	# LDFLAGS     += -static-libstdc++
	CFLAGS_SDL  += -DTARGET_PLATFORM_WINDOWS=1
else
	CFLAGS_SDL  := $(shell sdl2-config --cflags)
	LDFLAGS_SDL := $(shell sdl2-config --libs)
endif

CFLAGS_C   := $(CFLAGS) $(CFLAGS_SDL) -std=c17
CFLAGS_CXX := $(CFLAGS) $(CFLAGS_SDL) -std=c++17

LDFLAGS    += -lm -lpthread
LDFLAGS    += -lstdc++fs -std=c++17
LDFLAGS    += $(LDFLAGS_SDL)

SRCS := $(MAIN_SOURCES)

$(RELEASE_DIR)/$(MAIN_BINARY): $(MAIN_OBJS)
	@echo '===> CXX $@'
	@mkdir -p $(RELEASE_DIR)
	@$(CXX) $^ $(CFLAGS_CXX) $(LDFLAGS) -o $@

all:
	$(MAKE) TARGET_ARCH=linux-gcc-amd64
	$(MAKE) TARGET_ARCH=linux-gcc-x86
	$(MAKE) TARGET_ARCH=linux-clang-amd64
	$(MAKE) TARGET_ARCH=linux-clang-x86
	$(MAKE) TARGET_ARCH=win32
	$(MAKE) TARGET_ARCH=win64

-include $(MAIN_DEPS)

$(BUILD_DIR)/%.o: %.cc
	@echo '===> CXX $<'
	@mkdir -p $$(dirname $@)
	@$(CXX) $(CFLAGS_CXX) -c -MMD -o $@ $<

$(BUILD_DIR)/%.o: %.c
	@echo '===> CC $<'
	@mkdir -p $$(dirname $@)
	@$(CC) $(CFLAGS_C) -c -MMD -o $@ $<

clean:
	rm -rf build

install-build-dependencies:
	sudo dpkg --add-architecture i386
	sudo apt update
	sudo apt-get install -y \
		libsdl2-dev libsdl2-dev:i386 \
		build-essential \
		clang \
		gcc-multilib \
		g++-multilib \
		gcc-mingw-w64 gcc-mingw-w64-i686 \
		g++-mingw-w64 g++-mingw-w64-i686
