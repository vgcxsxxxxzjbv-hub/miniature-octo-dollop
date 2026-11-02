#!/usr/bin/env bash
set -e

NDK="${ANDROID_NDK:-$HOME/Android/Sdk/ndk/26.1.10909125}"
TC="$NDK/build/cmake/android.toolchain.cmake"
STRIP="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip"

# ── FA header ──
mkdir -p generated
if [ ! -f generated/fa_solid.h ]; then
  curl -sL --max-time 30 \
    "https://cdn.jsdelivr.net/npm/@fortawesome/fontawesome-free@5.15.4/webfonts/fa-solid-900.ttf" \
    -o generated/fa.ttf || true
  if [ -s generated/fa.ttf ]; then
    xxd -i -n SPINOX_FA_DATA generated/fa.ttf | \
      sed 's/^unsigned char/static const unsigned char/; s/^unsigned int/static const size_t/' \
      > generated/fa_solid.h
    echo "FA ready"
  else
    printf '#pragma once\n#include <cstddef>\nstatic const unsigned char SPINOX_FA_DATA[] = {0};\nstatic const size_t SPINOX_FA_SIZE = 0;\n' \
      > generated/fa_solid.h
    echo "FA fallback"
  fi
fi

declare -A PLATFORM=([arm64-v8a]=android-24 [armeabi-v7a]=android-21 [x86_64]=android-24)
rm -rf library && mkdir -p library

for abi in arm64-v8a armeabi-v7a x86_64; do
  echo ">>> $abi"
  cmake -B "build-$abi" \
    -DCMAKE_TOOLCHAIN_FILE="$TC" \
    -DANDROID_ABI="$abi" \
    -DANDROID_PLATFORM="${PLATFORM[$abi]}" \
    -DANDROID_STL=c++_static \
    -DCMAKE_BUILD_TYPE=Release
  cmake --build "build-$abi" -j"$(nproc)" || {
    echo "build failed: $abi"; continue
  }
  "$STRIP" --strip-all "library/$abi/libspinox.so" 2>/dev/null || true
done

find library -type f -name "*.so" -exec file {} \;
