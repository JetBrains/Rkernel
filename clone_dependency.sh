#!/bin/bash

set -e

cd "$(dirname "$0")"

if [ ! -d Rcpp ]; then
  git clone --single-branch https://github.com/RcppCore/Rcpp.git Rcpp
  cd Rcpp
  git checkout a2f5cfca5b9d4ff8017f84a3428375e69665a505
  cd ..
fi

if [ ! -d vcpkg ]; then
  git clone --single-branch --branch 2019.12  https://github.com/microsoft/vcpkg.git
fi


