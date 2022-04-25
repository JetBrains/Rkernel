#!/bin/bash

case "$OS" in
Linux)
  PACKAGE_NAME="$ARCH-linux"
  ;;
Darwin)
  PACKAGE_NAME="$ARCH-osx"
  MACOSX_DEPLOYMENT_TARGET=10.13
  export MACOSX_DEPLOYMENT_TARGET
  ;;
*)
  echo "Unsupported os '$OS'"
  exit 1
  ;;
esac

export PACKAGE_NAME

