cmake_minimum_required(VERSION 3.18)
project(spinox CXX C)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

include(FetchContent)
set(DOBBY_GENERATE_SHARED OFF CACHE BOOL "" FORCE)
set(DOBBY_GENERATE_STATIC  ON  CACHE BOOL "" FORCE)
set(DOBBY_DEBUG OFF CACHE BOOL "" FORCE)
FetchContent_Declare(dobby GIT_REPOSITORY https://github.com/jmpews/Dobby.git GIT_TAG master GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(dobby)
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.90.9 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(imgui)

set(I ${imgui_SOURCE_DIR})
add_library(imgui STATIC
  ${I}/imgui.cpp ${I}/imgui_draw.cpp ${I}/imgui_tables.cpp ${I}/imgui_widgets.cpp
  ${I}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC ${I} ${I}/backends)

add_library(spinox SHARED
  src/log.cpp src/il2cpp_resolver.cpp src/hooks.cpp src/jni_util.cpp
  src/overlay.cpp src/editor.cpp src/input.cpp src/entry.cpp
  src/memory.cpp src/scanner.cpp src/universal.cpp src/config.cpp
  src/array_edit.cpp src/codegen.cpp src/preset.cpp)
target_include_directories(spinox PRIVATE include)
target_link_libraries(spinox PRIVATE dobby imgui log android EGL GLESv3 dl)

if (ANDROID_ABI STREQUAL "arm64-v8a")
  target_compile_options(spinox PRIVATE -O2 -fvisibility=hidden -march=armv8-a)
  target_link_options(spinox PRIVATE -Wl,-z,max-page-size=16384)
elseif (ANDROID_ABI STREQUAL "armeabi-v7a")
  target_compile_options(spinox PRIVATE -O2 -fvisibility=hidden -march=armv7-a -mfpu=neon -mfloat-abi=softfp)
  target_link_options(spinox PRIVATE -Wl,-z,max-page-size=4096)
elseif (ANDROID_ABI STREQUAL "x86_64")
  target_compile_options(spinox PRIVATE -O2 -fvisibility=hidden)
  target_link_options(spinox PRIVATE -Wl,-z,max-page-size=16384)
endif()

if (ANDROID_ABI STREQUAL "arm64-v8a" OR ANDROID_ABI STREQUAL "x86_64")
  target_compile_definitions(spinox PRIVATE SPINOX_BITS=64)
else()
  target_compile_definitions(spinox PRIVATE SPINOX_BITS=32)
endif()

set(FA_TTF "${CMAKE_BINARY_DIR}/fa-solid-900.ttf")
if(NOT EXISTS "${FA_TTF}")
  file(DOWNLOAD
    "https://cdn.jsdelivr.net/npm/@fortawesome/fontawesome-free@5.15.4/webfonts/fa-solid-900.ttf"
    "${FA_TTF}" SHOW_PROGRESS TLS_VERIFY ON STATUS FA_DL)
  list(GET FA_DL 0 FA_CODE)
  if(NOT FA_CODE EQUAL 0)
    file(WRITE "${FA_TTF}" "")
  endif()
endif()
file(SIZE "${FA_TTF}" FA_SIZE)
if(FA_SIZE GREATER 1000)
  file(READ "${FA_TTF}" FA_HEX HEX)
  string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," FA_BYTES "${FA_HEX}")
  file(WRITE "${CMAKE_BINARY_DIR}/fa_solid.h"
    "#pragma once\n#include <cstddef>\n"
    "static const unsigned char SPINOX_FA_DATA[] = {${FA_BYTES}};\n"
    "static const size_t SPINOX_FA_SIZE = ${FA_SIZE};\n")
  target_include_directories(spinox PRIVATE "${CMAKE_BINARY_DIR}")
  target_compile_definitions(spinox PRIVATE SPINOX_FA_AVAILABLE=1)
else()
  target_compile_definitions(spinox PRIVATE SPINOX_FA_AVAILABLE=0)
endif()

set_target_properties(spinox PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/library/${ANDROID_ABI}"
  LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/library/${ANDROID_ABI}"
  LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/library/${ANDROID_ABI}"
  OUTPUT_NAME "spinox")

if (CMAKE_BUILD_TYPE STREQUAL "Release" AND CMAKE_STRIP)
  add_custom_command(TARGET spinox POST_BUILD
    COMMAND ${CMAKE_STRIP} --strip-all "${CMAKE_SOURCE_DIR}/library/${ANDROID_ABI}/libspinox.so")
endif()
