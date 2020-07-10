#!/bin/bash
set -euo pipefail

CMAKE="${CMAKE:-cmake}"

./clone_dependency.sh

if [ ! -d grpc ]; then
  ./build_grpc.sh
fi

if [ ! -d gens/protos ]; then
  mkdir -p gens/protos
fi

if [ -d ../protos ]; then
  rm -rf Rkernel-proto
  cp -rp ../protos Rkernel-proto
fi

chmod -R +x grpc/tools
if [[ "$OSTYPE" == "linux-gnu" ]]; then
  PACKAGE_NAME="x64-linux"
  IS_MACOS=""
else
  PACKAGE_NAME="x64-osx"
  IS_MACOS=true
fi

mkdir -p build
cd build
if [ -e "crashpad/crashpad" ]; then
  $CMAKE -Wdev -DCRASHPAD_DIR="$(pwd)/crashpad" .. "$@"
else
  $CMAKE -Wdev .. "$@"
fi
make -j4
if [ -n "$IS_MACOS" ]; then
  OLDNAME=$(otool -L rwrapper | grep "libR.dylib" | awk '{ print $1 }')
  install_name_tool -change $OLDNAME @library_path/libR.dylib rwrapper
fi
mv rwrapper ../rwrapper-$PACKAGE_NAME

cd ..
if [ ! -f fsnotifier-linux ]; then
  curl https://github.com/JetBrains/intellij-community/raw/master/bin/linux/fsnotifier -o fsnotifier-linux
  chmod +x fsnotifier-linux
fi
if [ ! -f fsnotifier-osx ]; then
  curl https://github.com/JetBrains/intellij-community/raw/master/bin/mac/fsnotifier -o fsnotifier-osx
  chmod +x fsnotifier-osx
fi
if [ ! -f fsnotifier-win.exe ]; then
  curl https://github.com/JetBrains/intellij-community/raw/master/bin/win/fsnotifier.exe -o fsnotifier-win.exe
fi
