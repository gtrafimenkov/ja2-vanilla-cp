cmake_minimum_required(VERSION 3.10)

# Using the same debug library as google test.
# Otherwise it will not link.
if(WIN32)
  # https://cmake.org/cmake/help/latest/variable/CMAKE_MSVC_RUNTIME_LIBRARY.html
  # https://stackoverflow.com/questions/10113017/setting-the-msvc-runtime-in-cmake#56776146
  cmake_policy(SET CMP0091 NEW)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project(ja2vcp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# suppressing some warning temporarily
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # add_compile_options(-Wno-unused-parameter)  # Example for GCC
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # add_compile_options(-Wno-unused-parameter)  # Example for Clang
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    message(STATUS "Compiler: MSVC")

    add_compile_options(/utf-8)

    add_compile_options(/wd4018) # signed/unsigned mismatch
    add_compile_options(/wd4200) # nonstandard extension used: zero-sized array in struct/union
    add_compile_options(/wd4244) # conversion from 'type1' to 'type2', possible loss of data
    add_compile_options(/wd4267) # conversion from 'size_t' to 'type', possible loss of data
    add_compile_options(/wd4996) # deprecated stuff in utf8cpp library
endif()

add_subdirectory(libs/sdl2 EXCLUDE_FROM_ALL)
add_subdirectory(libs/googletest)

file(GLOB_RECURSE SRC_FILES ja2/Build/*.cc libs/slog/slog/*.c libs/smacker/libsmacker/*.c)

add_executable(ja2vcp ${SRC_FILES})
target_include_directories(ja2vcp PRIVATE ja2/Build)
target_include_directories(ja2vcp PRIVATE libs/googletest/googletest/include)
target_include_directories(ja2vcp PRIVATE libs/slog)
target_include_directories(ja2vcp PRIVATE libs/smacker)
target_include_directories(ja2vcp PRIVATE libs/utf8cpp/source)

if(WIN32)
    if(TARGET SDL2::SDL2main)
        target_link_libraries(ja2vcp PRIVATE SDL2::SDL2main)
    endif()
endif()

target_link_libraries(ja2vcp PRIVATE SDL2::SDL2-static gtest)
# target_link_libraries(my_tests gtest gtest_main)
