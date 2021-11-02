#!/bin/bash

set -e

cd "$(dirname "$0")"

if [ ! -d vcpkg ]; then
  if [ "$OS" == "Darwin" ] && [ "$ARCH" == "arm64" ]; then
    git clone --single-branch --branch 2021.05.12 --depth 1 https://github.com/microsoft/vcpkg.git
  else
    git clone --single-branch --branch 2019.12 --depth 1 https://github.com/microsoft/vcpkg.git
  fi
fi
