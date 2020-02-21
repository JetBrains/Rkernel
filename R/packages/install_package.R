#!/usr/bin/env Rscript

args = commandArgs(TRUE)

if (length(args) != 0) {
  warning("Usage: install_package.R")
  quit(save = "no", status = 1, runLast = FALSE)
}

devtools::install(reload = FALSE, upgrade = "never", keep_source = TRUE)
