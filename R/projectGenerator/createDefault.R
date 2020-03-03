#  Rkernel is an execution kernel for R interpreter
#  Copyright (C) 2019 JetBrains s.r.o.
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https:#www.gnu.org/licenses/>.


createDefault <- function(packageDirName, packageParentDirPath) {
  assign('JB.fake.fun', function() {}, envir = .GlobalEnv)
  utils::package.skeleton(name = packageDirName, path = packageParentDirPath, list = 'JB.fake.fun', force = TRUE)
  rm('JB.fake.fun', envir = .GlobalEnv)
  dirPath <- file.path(packageParentDirPath, packageDirName)
  unlink(file.path(dirPath, 'R'  , 'JB.fake.fun.R'))
  unlink(file.path(dirPath, 'man', 'JB.fake.fun.Rd'))
  instrumentPackageManual(packageDirName, packageParentDirPath)
}

instrumentPackageManual <- function(packageDirName, packageParentDirPath) {
  manualPath <- getManualPath(packageDirName, packageParentDirPath)
  lines <- readLines(manualPath)
  replacedRange <- findReplacedRange(lines)
  if (!is.null(replacedRange)) {
    replacingValue <- "# simple examples of the most important functions"
    replacedLines <- replace(lines, replacedRange, replacingValue)
    writeLines(replacedLines, manualPath)
  }
}

getManualPath <- function(packageDirName, packageParentDirPath) {
  manualName <- paste0(packageDirName, "-package.Rd")
  file.path(packageParentDirPath, packageDirName, "man", manualName)
}

findReplacedRange <- function(lines) {
  firstIndex <- NULL
  lastIndex <- NULL
  for (i in 1:length(lines)) {
    line <- lines[i]
    if (identical(substr(line, 1, 10), "\\examples{")) {
      firstIndex <- i
    } else if (!is.null(firstIndex) && identical(substr(line, 1, 1), "}")) {
      secondIndex <- i
      break
    }
  }
  if (!is.null(firstIndex) && !is.null(secondIndex) && secondIndex - firstIndex > 1) {
    replacedRange <- (firstIndex + 1):(secondIndex - 1)
  } else {
    NULL
  }
}

args <- commandArgs(TRUE)
createDefault(args[1], args[2])