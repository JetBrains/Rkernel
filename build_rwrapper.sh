#!/bin/bash
set -euo pipefail

CMAKE="${CMAKE:-cmake}"

. ./env_os.sh

if [[ "$OS" == "Darwin" ]] && [[ "$ARCH" == "arm64" ]]; then
  rm -rf build
  rm -rf gens
  rm -rf grpc
  rm -rf Rkernel-proto
  rm -rf vcpkg
fi

./clone_dependency.sh

if [ ! -d grpc ]; then
  ./build_grpc.sh
fi

. ./env.sh


if [ ! -d gens/protos ]; then
  mkdir -p gens/protos
fi

if [ -d ../protos ]; then
  rm -rf Rkernel-proto
  cp -rp ../protos Rkernel-proto
fi


chmod -R +x grpc/tools || true

mkdir -p build
cd build

$CMAKE -Wdev .. "$@"
make -j1
if [[ "$OS" == "Darwin" ]]; then
  OLDNAME=$(otool -L rwrapper | grep "libR.dylib" | awk '{ print $1 }')
  install_name_tool -change $OLDNAME @library_path/libR.dylib rwrapper
fi
mv rwrapper ../rwrapper-$PACKAGE_NAME

cd ..
./get_fsnotifier.sh
