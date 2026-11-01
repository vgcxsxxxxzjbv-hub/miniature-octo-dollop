#!/usr/bin/env bash
set -e
NDK="${ANDROID_NDK:-$HOME/Android/Sdk/ndk/26.1.10909125}"
TC="$NDK/build/cmake/android.toolchain.cmake"
STRIP="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip"
declare -A PLATFORM=([arm64-v8a]=android-24 [armeabi-v7a]=android-21 [x86_64]=android-24)
rm -rf library && mkdir -p library
for abi in arm64-v8a armeabi-v7a x86_64; do
  cmake -B "build-$abi" -DCMAKE_TOOLCHAIN_FILE="$TC" -DANDROID_ABI="$abi" \
    -DANDROID_PLATFORM="${PLATFORM[$abi]}" -DANDROID_STL=c++_static -DCMAKE_BUILD_TYPE=Release
  cmake --build "build-$abi" -j"$(nproc)"
  "$STRIP" --strip-all "library/$abi/libspinox.so" 2>/dev/null || true
done
find library -type f -name "*.so" -exec file {} \;
