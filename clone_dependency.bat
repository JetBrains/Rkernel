cd /D "%~dp0"

if not exist Rcpp (
  mkdir Rcpp
)

if not exist vcpkg (
  git clone --single-branch --branch 2022.05.10  https://github.com/microsoft/vcpkg.git
  git -C vcpkg apply --reject --whitespace=fix --ignore-space-change --ignore-whitespace ../vcpkg-patches/nasm.diff
)


