#!/usr/bin/env Rscript

args = commandArgs(TRUE)

if (length(args) != 0) {
  warning("Usage: setup_tests.R")
  quit(save = "no", status = 1, runLast = FALSE)
}

usethis::use_testthat()
