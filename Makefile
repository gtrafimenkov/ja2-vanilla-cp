.PHONY: build build-release clean run test

build:
	cmake -B build -DCMAKE_BUILD_TYPE=Debug
	cmake --build build --parallel

build-release:
	cmake -B build-release -DCMAKE_BUILD_TYPE=Release
	cmake --build build-release --parallel

test:
	./build/ja2vcp --unittests

clean:
	rm -rf build

run:
	cd build && ./ja2vcp

# # Copyright (c) 2021 Gennady Trafimenkov
# #
# # Permission to use, copy, modify, and/or distribute this software for any
# # purpose with or without fee is hereby granted.
# #
# # THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
# # REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# # AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
# # INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# # LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
# # OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# # PERFORMANCE OF THIS SOFTWARE.


# ifneq "$(shell uname)" "Linux"
#   $(error This build script must run on Linux)
# endif

# BUILD_INFO     := $(shell git describe --long --tags --always || echo "unknown")

# # By default building binary for whatever system the build system is running
# TARGET_ARCH    ?=

# # Place for object and binary files
# RELEASE_DIR    := build
# ifeq "$(TARGET_ARCH)" ""
# BUILD_DIR      := build/default
# else
# BUILD_DIR      := build/$(TARGET_ARCH)
# endif

# # The main binary will be built from content of src folder plus the static library
# ifeq "$(TARGET_ARCH)" ""
# MAIN_BINARY  := ja2-vanilla-cp
# else
# MAIN_BINARY  := ja2-vanilla-cp-$(TARGET_ARCH)
# endif

# MAIN_SOURCES := $(shell find ja2 -name '*.cc')
# MAIN_SOURCES += $(shell find libs/slog/slog -name '*.c')
# MAIN_SOURCES += libs/smacker/libsmacker/smacker.c
# MAIN_SOURCES += libs/smacker/libsmacker/smk_hufftree.c
# MAIN_SOURCES += libs/smacker/libsmacker/smk_bitstream.c
# MAIN_OBJS0   := $(filter %.o, $(MAIN_SOURCES:.c=.o) $(MAIN_SOURCES:.cc=.o) $(MAIN_SOURCES:.cpp=.o))
# MAIN_OBJS    := $(addprefix $(BUILD_DIR)/,$(MAIN_OBJS0))
# MAIN_DEPS    := $(MAIN_OBJS:.o=.d)

# ifeq "$(TARGET_ARCH)" "linux-gcc-amd64"
# 	AR             := ar
# 	CC             := gcc
# 	CXX            := g++
# 	CFLAGS         += -m64
# endif

# ifeq "$(TARGET_ARCH)" "linux-gcc-x86"
# 	AR             := ar
# 	CC             := gcc
# 	CXX            := g++
# 	CFLAGS         += -m32
# endif

# ifeq "$(TARGET_ARCH)" "linux-clang-amd64"
# 	AR             := ar
# 	CC             := clang
# 	CXX            := clang++
# 	CFLAGS         += -m64
# endif

# ifeq "$(TARGET_ARCH)" "linux-clang-x86"
# 	AR             := ar
# 	CC             := clang
# 	CXX            := clang++
# 	CFLAGS         += -m32
# endif

# ifeq "$(TARGET_ARCH)" "win32"
# 	MINGW_ARCH     := i686-w64-mingw32
# endif

# ifeq "$(TARGET_ARCH)" "win64"
# 	MINGW_ARCH     := x86_64-w64-mingw32
# endif

# ifdef MINGW_ARCH
# 	AR             := $(MINGW_ARCH)-ar
# 	CC             := $(MINGW_ARCH)-gcc
# 	CXX            := $(MINGW_ARCH)-g++
# 	LOCAL_SDL_LIB  := libs/SDL2-devel-2.0.14-mingw/SDL2-2.0.14/$(MINGW_ARCH)
# endif

# ifdef DEBUGBUILD
# CFLAGS += -g
# endif

# CFLAGS += -DBUILD_INFO=\"$(BUILD_INFO)\"

# CFLAGS += -I ja2
# CFLAGS += -I libs/slog
# CFLAGS += -I libs/smacker
# CFLAGS += -I libs/utf8cpp/source

# ifdef LOCAL_SDL_LIB
# 	CFLAGS_SDL  := $(shell $(LOCAL_SDL_LIB)/bin/sdl2-config --cflags | sed s@/opt/local/$(MINGW_ARCH)/@$(LOCAL_SDL_LIB)/@g)
# 	# if you want to leave the console window (too see the log), remove flag "-mwindows" from LDFLAGS_SDL
# 	LDFLAGS_SDL := $(shell $(LOCAL_SDL_LIB)/bin/sdl2-config --static-libs | sed s@/opt/local/$(MINGW_ARCH)/@$(LOCAL_SDL_LIB)/@g)
# 	# making completely static build, SDL2.dll will not be required
# 	LDFLAGS     += -static
# 	# if you want partially static build (SDL2.dll required, libstdc++6.dll is not)
# 	# LDFLAGS     += -static-libstdc++
# else
# 	CFLAGS_SDL  := $(shell sdl2-config --cflags)
# 	LDFLAGS_SDL := $(shell sdl2-config --libs)
# endif

# CFLAGS_C   := $(CFLAGS) $(CFLAGS_SDL) -std=c17
# CFLAGS_CXX := $(CFLAGS) $(CFLAGS_SDL) -std=c++17

# LDFLAGS    += -lm -lpthread
# LDFLAGS    += -lstdc++fs -std=c++17
# LDFLAGS    += $(LDFLAGS_SDL)

# SRCS := $(MAIN_SOURCES)

# $(RELEASE_DIR)/$(MAIN_BINARY): $(MAIN_OBJS)
# 	@echo '===> CXX $@'
# 	@mkdir -p $(RELEASE_DIR)
# 	@$(CXX) $^ $(CFLAGS_CXX) $(LDFLAGS) -o $@

# all:
# 	$(MAKE) TARGET_ARCH=linux-gcc-amd64
# 	$(MAKE) TARGET_ARCH=linux-gcc-x86
# 	$(MAKE) TARGET_ARCH=linux-clang-amd64
# 	$(MAKE) TARGET_ARCH=linux-clang-x86
# 	$(MAKE) TARGET_ARCH=win32
# 	$(MAKE) TARGET_ARCH=win64

# -include $(MAIN_DEPS)

# $(BUILD_DIR)/%.o: %.cc
# 	@echo '===> CXX $<'
# 	@mkdir -p $$(dirname $@)
# 	@$(CXX) $(CFLAGS_CXX) -c -MMD -o $@ $<

# $(BUILD_DIR)/%.o: %.c
# 	@echo '===> CC $<'
# 	@mkdir -p $$(dirname $@)
# 	@$(CC) $(CFLAGS_C) -c -MMD -o $@ $<

# clean:
# 	rm -rf build

# install-build-dependencies-linux:
# 	sudo apt-get install -y \
# 		libsdl2-dev \
# 		build-essential

# install-build-dependencies-clang:
# 	sudo apt-get install -y \
# 		clang

# install-build-dependencies-x86:
# 	sudo dpkg --add-architecture i386
# 	sudo apt update
# 	sudo apt-get install -y \
# 		libsdl2-dev:i386 \
# 		gcc-multilib \
# 		g++-multilib

# install-build-dependencies-win:
# 	sudo apt-get install -y \
# 		gcc-mingw-w64 gcc-mingw-w64-i686 \
# 		g++-mingw-w64 g++-mingw-w64-i686

# CLANG_FORMATTER ?= ~/bin/clang+llvm-11.1.0-x86_64-linux-gnu-ubuntu-16.04/bin/clang-format

# format:
# 	find ja2 \( -iname '*.c' -o -iname '*.cc' -o -iname '*.cpp' -o -iname '*.h' \) \
# 		| xargs $(CLANG_FORMATTER) -i --style=file

# format-modified:
# 	git status --porcelain | egrep -e '[.](c|cc|cpp|h)$$' | awk '{print $$2}' \
# 		| xargs $(CLANG_FORMATTER) -i --style=file
