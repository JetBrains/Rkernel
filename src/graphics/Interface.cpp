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


#include <iostream>
#include <string>

#include <Rcpp.h>

#include "DeviceManager.h"

using namespace Rcpp;
using namespace graphics;

// [[Rcpp::plugins(cpp11)]]
// [[Rcpp::export]]
SEXP jetbrains_ther_device_init(CharacterVector snapshotDir, double width, double height, int resolution, bool inMemory) {
  auto path = as<std::string>(snapshotDir[0]);
  auto screenParameters = ScreenParameters{width, height, resolution};
  DeviceManager::getInstance()->initNew(path, screenParameters, inMemory);
  return R_NilValue;
}

SEXP jetbrains_ther_device_shutdown() {
  DeviceManager::getInstance()->shutdownLast();
  return R_NilValue;
}

SEXP jetbrains_ther_device_record() {
  auto active = DeviceManager::getInstance()->getActive();
  if (active) {
    active->recordLast();
  } else {
    std::cerr << "jetbrains_ther_device_record(): nothing to record. Ignored\n";
  }
  return R_NilValue;
}

SEXP jetbrains_ther_device_rescale(int snapshotNumber, double width, double height, int resolution) {
  auto active = DeviceManager::getInstance()->getActive();
  auto isRescaled = false;
  if (active) {
    if (active->isOnlineRescalingEnabled()) {
      auto newParameters = graphics::ScreenParameters{width, height, resolution};
      if (snapshotNumber >= 0) {
        isRescaled = active->rescaleByNumber(snapshotNumber, newParameters);
      } else {
        isRescaled = active->rescaleAllLast(newParameters);
      }
    } else {
      std::cerr << "jetbrains_ther_device_rescale(): current device cannot be rescaled. Ignored\n";
    }
  } else {
    std::cerr << "jetbrains_ther_device_rescale(): nothing to rescale. Ignored\n";
  }
  return Rcpp::wrap(isRescaled);
}

SEXP jetbrains_ther_device_rescale_stored(const std::string& parentDirectory, int snapshotNumber, int snapshotVersion,
                                          double width, double height, int resolution) {
  auto active = DeviceManager::getInstance()->getActive();
  if (active) {
    auto newParameters = ScreenParameters{width, height, resolution};
    return Rcpp::wrap(active->rescaleByPath(parentDirectory, snapshotNumber, snapshotVersion, newParameters));
  } else {
    return Rcpp::wrap(false);
  }
}
