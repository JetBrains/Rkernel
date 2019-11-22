#!/bin/bash

git clone --depth 1 https://chromium.googlesource.com/chromium/tools/depot_tools.git depot_tools
DEPOT_TOOLS_DIR=$(realpath depot_tools)
export PATH=$PATH:$DEPOT_TOOLS_DIR
fetch crashpad 
sync crashpad
cd crashpad
$DEPOT_TOOLS_DIR/gn gen out/Default

if [[ "$OSTYPE" = "darwin"* ]]; then
  # on macos, we need to specify the minimum os target to support
  echo "mac_deployment_target=\"10.12\"" > out/Default/args.gn
  $DEPOT_TOOLS_DIR/gn gen out/Default
fi

$DEPOT_TOOLS_DIR/ninja -C out/Default

# fix up permissions (if run as root, bin dir will be unreadable by others)
chmod -R 755 out

