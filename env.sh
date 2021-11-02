#!/bin/bash

MACOSX_DEPLOYMENT_TARGET=10.13

case "$OS" in
Linux)
  PACKAGE_NAME="$ARCH-linux"
  ;;
Darwin)
  PACKAGE_NAME="$ARCH-osx"
  [[ "$ARCH" == "x64" ]] && export MACOSX_DEPLOYMENT_TARGET=10.13
  ;;
*)
  echo "Unsupported os '$OS'"
  exit 1
  ;;
esac

export PACKAGE_NAME
export MACOSX_DEPLOYMENT_TARGET
