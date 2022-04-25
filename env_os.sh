#!/bin/bash
set -euo pipefail

case $(uname -m) in
x86_64) ARCH="x64" ;;
arm64|aarch64) ARCH="arm64" ;;
*)
  echo "Unknown architecture $(uname -m)" >&2
  exit 1
  ;;
esac

OS="$(uname -s)"

export OS
export ARCH
