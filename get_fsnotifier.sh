#!/bin/bash
set -euo pipefail

if [ ! -f fsnotifier-linux ]; then
  curl -fsSL https://intellij.com/fsnotifier-linux-amd64 -o fsnotifier-linux
  chmod +x fsnotifier-linux
fi
if [ ! -f fsnotifier-linux-aarch64 ]; then
  curl -fsSL https://intellij.com/fsnotifier-linux-aarch64 -o fsnotifier-linux-aarch64
  chmod +x fsnotifier-linux-aarch64
fi
if [ ! -f fsnotifier-osx ]; then
  curl -fsSL https://intellij.com/fsnotifier-macos -o fsnotifier-osx
  chmod +x fsnotifier-osx
fi
if [ ! -f fsnotifier-win.exe ]; then
  curl -fsSL https://intellij.com/fsnotifier-win-amd64 -o fsnotifier-win.exe
fi
