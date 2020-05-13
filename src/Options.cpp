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

#include "Options.h"

CommandLineOptions commandLineOptions;

void CommandLineOptions::parse(int argc, char* argv[]) {
  cxxopts::Options options("RWrapper", "Execution kernel for R interpreter");
  options.add_options()
      ("h,help", "Show help and exit")
      ("with-timeout", "Terminate RWrapper if no RPCs were received for a minute")
      ("crash-report-file", "File for saving crash report", cxxopts::value<std::string>());
  try {
    auto result = options.parse(argc, argv);
    if (result["help"].as<bool>()) {
      std::cout << options.help();
      exit(0);
    }
    withTimeout = result["with-timeout"].as<bool>();
    if (result.count("crash-report-file")) {
      crashReportFile = result["crash-report-file"].as<std::string>();
    }
  } catch (cxxopts::OptionParseException const& e) {
    std::cerr << e.what() << "\n";
    exit(1);
  }
}
