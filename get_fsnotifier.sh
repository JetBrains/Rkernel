#!/bin/bash
set -euo pipefail

if [ ! -f fsnotifier-linux ]; then
  curl https://raw.githubusercontent.com/JetBrains/intellij-community/master/bin/linux/fsnotifier64 -o fsnotifier-linux
  chmod +x fsnotifier-linux
fi
if [ ! -f fsnotifier-osx ]; then
  curl https://raw.githubusercontent.com/JetBrains/intellij-community/master/bin/mac/fsnotifier -o fsnotifier-osx
  chmod +x fsnotifier-osx
fi
if [ ! -f fsnotifier-win.exe ]; then
  curl https://raw.githubusercontent.com/JetBrains/intellij-community/master/bin/win/fsnotifier.exe -o fsnotifier-win.exe
fi
