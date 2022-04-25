#!/bin/bash
set -euo pipefail

# shellcheck disable=SC2034
export VCPKG_DISABLE_METRICS=1
cd vcpkg
./bootstrap-vcpkg.sh -disableMetrics

. ../env_os.sh
. ../env.sh

./vcpkg install "grpc:$PACKAGE_NAME"
./vcpkg install "tiny-process-library:$PACKAGE_NAME"
rm -rf ../grpc
mkdir ../grpc

if which rsync &>/dev/null; then
  rsync -r --exclude=CONTROL --exclude=BUILD_INFO packages/*_"$PACKAGE_NAME"/ ../grpc
else
  # shellcheck disable=SC2010
  for d in $(ls packages | grep -e "_$PACKAGE_NAME"); do
    echo "Copying from $d"
    cp -r packages/"$d"/* ../grpc
  done
  rm -f ../grpc/CONTROL ../grpc/BUILD_INFO
fi

cd ..
