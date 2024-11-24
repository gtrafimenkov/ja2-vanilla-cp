cmake_minimum_required(VERSION 3.5)
project(ja2vcp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
