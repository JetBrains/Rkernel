#!/bin/bash
set -euo pipefail

if [ ! -f fsnotifier-linux ]; then
  curl https://raw.githubusercontent.com/JetBrains/intellij-community/master/bin/linux/fsnotifier -o fsnotifier-linux
  chmod +x fsnotifier-linux
fi
if [ ! -f fsnotifier-linux-aarch64 ]; then
  curl https://raw.githubusercontent.com/JetBrains/intellij-community/master/bin/linux/fsnotifier-aarch64 -o fsnotifier-linux-aarch64
  chmod +x fsnotifier-linux-aarch64
fi
if [ ! -f fsnotifier-osx ]; then
  curl https://raw.githubusercontent.com/JetBrains/intellij-community/master/bin/mac/fsnotifier -o fsnotifier-osx
  chmod +x fsnotifier-osx
fi
if [ ! -f fsnotifier-win.exe ]; then
  curl https://raw.githubusercontent.com/JetBrains/intellij-community/master/bin/win/fsnotifier.exe -o fsnotifier-win.exe
fi
