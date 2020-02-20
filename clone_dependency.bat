cd /D "%~dp0"

if not exist Rcpp (
  git clone --single-branch https://github.com/RcppCore/Rcpp.git Rcpp
  cd Rcpp
  git checkout a2f5cfca5b9d4ff8017f84a3428375e69665a505
  cd ..
)

if not exist vcpkg (
  git clone --single-branch --branch 2019.12  https://github.com/microsoft/vcpkg.git
)


