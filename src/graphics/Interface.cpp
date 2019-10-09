//  Rkernel is an execution kernel for R interpreter
//  Copyright (C) 2019 JetBrains s.r.o.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include <string>

#include <Rcpp.h>

#include "MasterDevice.h"
#include "SlaveDevice.h"


using namespace Rcpp;

// [[Rcpp::plugins(cpp11)]]
// [[Rcpp::export]]
SEXP jetbrains_ther_device_init(CharacterVector snapshotDir, double width, double height, int resolution, double scaleFactor) {
  auto path = as<std::string>(snapshotDir[0]);
  jetbrains::ther::device::master::init(path, { width, height, resolution }, scaleFactor);
  return R_NilValue;
}

using namespace Rcpp;

SEXP jetbrains_ther_device_dump() {
    jetbrains::ther::device::master::dumpAndMoveNext();
    return R_NilValue;
}

SEXP jetbrains_ther_device_rescale(int snapshotNumber, double width, double height) {
  jetbrains::ther::device::master::rescale(snapshotNumber, width, height);
  return R_NilValue;
}
