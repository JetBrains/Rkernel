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
  export MACOSX_DEPLOYMENT_TARGET=10.13
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
./get_fsnotifier.sh
