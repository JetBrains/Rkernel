#!/usr/bin/env Rscript

args = commandArgs(TRUE)
cran <- FALSE
if (args[1] == "--as-cran") {
  cran <- TRUE
  args <- tail(args, n = -1L)
}
# Note: it doesn't have the same effect to set `cran` to FALSE
# and pass "--as-cran" in `args`
devtools::check(document = FALSE, cran = cran, args = args)
