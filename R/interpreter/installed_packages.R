#!/usr/bin/env Rscript

args = commandArgs(TRUE)

if (length(args) != 0) {
  warning("Usage: installed_packages.R")
  quit(save = "no", status = 1, runLast = FALSE)
}

versions <- as.data.frame(installed.packages()[, c("Package", "Version", "Priority", "LibPath")])
cat(">>>RPLUGIN>>>")
with(versions, cat(paste(LibPath, Package, Version, Priority, sep = "\t"), sep = "\n"))
cat("<<<RPLUGIN<<<")