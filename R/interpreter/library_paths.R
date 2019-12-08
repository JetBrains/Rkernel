#!/usr/bin/env Rscript

args = commandArgs(TRUE)

if (length(args) != 0) {
  warning("Usage: library_paths.R")
  quit(save = "no", status = 1, runLast = FALSE)
}

cat(">>>RPLUGIN>>>")
writeLines(.libPaths())
cat("<<<RPLUGIN<<<")