#!/bin/bash
set -euo pipefail


VCPKG_DISABLE_METRICS=1
cd vcpkg
./bootstrap-vcpkg.sh

. ../env.sh

./vcpkg install "grpc:$PACKAGE_NAME"
./vcpkg install "tiny-process-library:$PACKAGE_NAME"
rm  -rf ../grpc
mkdir ../grpc
cp -r packages/*_$PACKAGE_NAME/* ../grpc/
cd ..


