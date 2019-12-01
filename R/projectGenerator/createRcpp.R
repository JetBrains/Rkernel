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


initKittenStub <- function() {
  Real.kitten <- pkgKitten::kitten
  utils::assignInNamespace('kitten',
    function(...) {
      utils::assignInNamespace('kitten', Real.kitten, 'pkgKitten')
      assign('Rcpp.fake.fun', function() {}, envir = parent.frame(2))
      call <- match.call()
      call[[1]] <- package.skeleton
      call[["force"]] <- TRUE
      call[["list"]] <- 'Rcpp.fake.fun'
      if ("example_code" %in% names(call)) {
        call[["example_code"]] <- NULL
      }
      eval(call)
    }, 'pkgKitten')
}

createDefault <- function(packageName, packageDirName, packageParentDirPath, extraOptions) {
  if ('pkgKitten' %in% .packages(TRUE)) {
    initKittenStub();
  }
  initCall <- paste0(packageName, '::', packageName, '.package.skeleton(name = \'', packageDirName,
          '\', path = \'', packageParentDirPath, '\', force = TRUE', extraOptions, ')')
  eval(parse(text = initCall), envir = parent.frame(1));
}

args <- commandArgs(TRUE)
createDefault(args[1], args[2], args[3], args[4])