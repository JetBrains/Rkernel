#!/bin/bash

set -e

cd "$(dirname "$0")"

if [ ! -d vcpkg ]; then
  git clone --single-branch --branch 2022.05.10 --depth 1 https://github.com/microsoft/vcpkg.git
fi
