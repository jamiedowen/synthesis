#!/usr/bin/env bash
# Builds the Android app (APK) of CJ Autoworx RevHead.
#
# JUCE's CMake API can't target Android, so this fetches JUCE, builds the Projucer,
# uses it to generate an Android Studio project from CJAutoworx.jucer, then runs Gradle.
#
# Needs: git, cmake, a C++ compiler, Java 17 and the Android SDK (ANDROID_HOME set).
# Gradle downloads the NDK version JUCE asks for if it isn't installed.
#
#   ./build-android.sh            # release APK (signed with the debug key, so it can be sideloaded)
#   CONFIG=Debug ./build-android.sh
#   PROJUCER=/path/to/Projucer ./build-android.sh   # skip building the Projucer
set -euo pipefail
cd "$(dirname "$0")"

JUCE_TAG=8.0.9   # keep in step with the top-level CMakeLists.txt
CONFIG="${CONFIG:-Release}"

if [ ! -d JUCE ]; then
    git clone --depth 1 --branch "$JUCE_TAG" https://github.com/juce-framework/JUCE.git JUCE
fi

if [ -z "${PROJUCER:-}" ]; then
    cmake -S JUCE -B JUCE/build -DJUCE_BUILD_EXTRAS=ON -DCMAKE_BUILD_TYPE=Release
    cmake --build JUCE/build --target Projucer --parallel
    PROJUCER="$(find JUCE/build -type f -name Projucer -perm -u+x | head -n 1)"
fi

"$PROJUCER" --resave CJAutoworx.jucer

# Release builds are signed with the standard Android debug key; make one if this machine has none
if [ ! -f "$HOME/.android/debug.keystore" ]; then
    mkdir -p "$HOME/.android"
    keytool -genkeypair -v -keystore "$HOME/.android/debug.keystore" -storepass android -alias androiddebugkey \
            -keypass android -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=Android Debug,O=Android,C=US"
fi

cd Builds/Android
./gradlew "assemble${CONFIG}" --no-daemon

find . -name "*.apk" -path "*outputs*" -print
