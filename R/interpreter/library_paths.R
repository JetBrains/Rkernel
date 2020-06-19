#!/usr/bin/env Rscript

args = commandArgs(TRUE)

if (length(args) != 0) {
  warning("Usage: library_paths.R")
  quit(save = "no", status = 1, runLast = FALSE)
}

cat(">>>RPLUGIN>>>")
for (path in .libPaths()) {
  cat(path, file.access(path, 2) == 0, sep = "!!!JETBRAINS_RPLUGIN!!!")
  cat("\n")
}
cat("<<<RPLUGIN<<<")