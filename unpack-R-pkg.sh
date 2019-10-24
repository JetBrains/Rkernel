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
  cp -r "$pkg/Payload"/* "$result$base"
  rm -rf "$pkg"
done
#chown -Rh root:admin "$result/Library/Frameworks/R.framework" || true
#chmod -R g+w "$result/Library/Frameworks/R.framework" || true
(
  cd "$result/usr/local/bin"
  ln -s "../../../Library/Frameworks/R.framework/Resources/bin/R" .
  # It makes no sense to link Rscript since it have path to Library/Frameworks/R.framework hardcoded
  #ln -s "../../../Library/Frameworks/R.framework/Resources/bin/Rscript" .
)

sed -i '' 's~DIR=/Library/Frameworks~DIR=$JAIL_ROOT/Library/Frameworks~' "$result/Library/Frameworks/R.framework/Resources/bin/R"
# ../../../Library/Frameworks/R.framework/Resources/bin/fc-cache

cd "$old"
mv "$tmpdir/$result" "$destination"
rm -rf "$tmpdir"
