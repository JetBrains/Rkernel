cd /D "%~dp0"

if not exist Rcpp (
  mkdir Rcpp
)

if not exist vcpkg (
  git clone --single-branch --branch 2019.12  https://github.com/microsoft/vcpkg.git
)


