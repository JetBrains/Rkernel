#!/bin/bash
set -euxo pipefail

function usage() {
  cat <<EOF
Usage: $0 PACKAGE [DESTINATION]

Unpacks specified macOS .pkg

Where
  PACKAGE     - is existing .pkg file
  DESTINATION - is non-exosting directory where result will be store
                defaults to PACKAGE without '.pkg' suffix
EOF
}

if [[ $# -lt 1 ]]; then
  usage
  exit 1
fi

package="$1"
destination="${2:-${package%.pkg}}"

if [ ! -f "$package" ]; then
  echo "Package file '$package' not found"
  usage
  exit 1
fi

if [ -d "$destination" ]; then
  echo "Destination $destination already exists"
  usage
  exit 1
fi

old="$(pwd)"

tmpdir="$destination.unpack.tmp"
rm -rf "$tmpdir"
mkdir -p "$(dirname "$tmpdir")"

pkgutil --expand-full "$package" "$tmpdir"

cd "$tmpdir"
result="_result_"
for pkg in ./*.pkg; do
  if [[ ! -d "$pkg" ]]; then
    continue
  fi
  #base="$(xmllint --xpath 'string(//@install-location)' "$pkg/PackageInfo")"
  base="$(grep -o 'install-location="[^".]*"' "$pkg/PackageInfo" | head -n1 | sed 's/install-location="\([^".]*\)"/\1/')"
  mkdir -p "$result$base"
  # use -R because R 4.0.0 uses absolute symlinks for X11 fonts, just ignore them
  cp -R "$pkg/Payload"/* "$result$base"
  rm -rf "$pkg"
done
#chown -Rh root:admin "$result/Library/Frameworks/R.framework" || true
#chmod -R g+w "$result/Library/Frameworks/R.framework" || true
if [[ -d "$result/usr/local/bin" ]]; then
  echo "X64 version, using $result/usr/local/bin"
  bin_dir="$result/usr/local/bin"
  up="../../.."
elif [[ -d "$result/opt/R/arm64/bin" ]]; then
  echo "ARM64 version, using $result/opt/R/arm64/bin"
  bin_dir="$result/opt/R/arm64/bin"
  up="../../../.."
fi

(
  cd "$bin_dir"
  ln -s "$up/Library/Frameworks/R.framework/Resources/bin/R" .
  # It makes no sense to link Rscript since it have path to Library/Frameworks/R.framework hardcoded
  #ln -s "$up/Library/Frameworks/R.framework/Resources/bin/Rscript" .
)

sed -i '' 's~DIR=/Library/Frameworks~DIR=$JAIL_ROOT/Library/Frameworks~' "$result/Library/Frameworks/R.framework/Resources/bin/R"
# $up/Library/Frameworks/R.framework/Resources/bin/fc-cache

if [[ "$(uname -m)" == "arm64" ]]; then
  R_BIN="$result/Library/Frameworks/R.framework/Resources/bin/exec/R"
  for l in $(otool -L "$R_BIN" | grep '\t/Library/Frameworks' | awk '{print $1}'); do
    install_name_tool -change "$l" "$result/$l" "$R_BIN"
  done
  otool -L "$R_BIN"
  # ad-hoc sign
  codesign --sign - --force --verbose "$R_BIN"
fi

cd "$old"
mv "$tmpdir/$result" "$destination"
rm -rf "$tmpdir"
