#!/bin/bash

set -e

cd "$(dirname "$0")"

if [ ! -d vcpkg ]; then
  git clone --single-branch --branch 2019.12  https://github.com/microsoft/vcpkg.git
fi


