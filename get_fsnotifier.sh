#!/bin/bash
set -euo pipefail

if [ ! -f fsnotifier-linux ]; then
  curl -L https://jb.gg/fsnotifier-linux-amd64 -o fsnotifier-linux
  chmod +x fsnotifier-linux
fi
if [ ! -f fsnotifier-linux-aarch64 ]; then
  curl -L https://jb.gg/fsnotifier-linux-aarch64 -o fsnotifier-linux-aarch64
  chmod +x fsnotifier-linux-aarch64
fi
if [ ! -f fsnotifier-osx ]; then
  curl -L https://jb.gg/fsnotifier-macos -o fsnotifier-osx
  chmod +x fsnotifier-osx
fi
if [ ! -f fsnotifier-win.exe ]; then
  curl -L https://jb.gg/fsnotifier-win-amd64 -o fsnotifier-win.exe
fi
