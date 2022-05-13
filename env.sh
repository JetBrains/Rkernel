#!/bin/bash
# shellcheck disable=SC2034

case "$OS" in
Linux)
  PACKAGE_NAME="$ARCH-linux"
  threads_count="$(if hash nproc 2>/dev/null; then nproc; else cat /proc/cpuinfo | grep processor | wc -l; fi)"
  ;;
Darwin)
  PACKAGE_NAME="$ARCH-osx"
  MACOSX_DEPLOYMENT_TARGET=10.13
  export MACOSX_DEPLOYMENT_TARGET
  threads_count="$(sysctl -n hw.logicalcpu)"
  ;;
*)
  echo "Unsupported os '$OS'"
  exit 1
  ;;
esac

export PACKAGE_NAME
