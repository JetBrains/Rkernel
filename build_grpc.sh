#!/bin/bash
set -euo pipefail

cd vcpkg
./bootstrap-vcpkg.sh

if [[ "$OSTYPE" == "linux-gnu" ]]; then
  PACKAGE_NAME="x64-linux"
else
  PACKAGE_NAME="x64-osx"
  export MACOSX_DEPLOYMENT_TARGET=10.13
fi

./vcpkg install "grpc:$PACKAGE_NAME"
./vcpkg install "tiny-process-library:$PACKAGE_NAME"
rm  -rf ../grpc
mkdir ../grpc
cp -r packages/*_$PACKAGE_NAME/* ../grpc/
cd ..


