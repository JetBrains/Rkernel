#!/usr/bin/env Rscript

args = commandArgs(TRUE)

if (length(args) != 0) {
  warning("Usage: update_rcpp_exports.R")
  quit(save = "no", status = 1, runLast = FALSE)
}

Rcpp::compileAttributes()
