#!/usr/bin/env Rscript

args = commandArgs(TRUE)

if (length(args) != 0) {
  warning("Usage: cran_mirrors.R")
  quit(save = "no", status = 1, runLast = FALSE)
}

mirrors <- getCRANmirrors()[, c('Name', 'URL')]
cat(">>>RPLUGIN>>>")
with(mirrors, cat(paste(Name, URL, sep = "\t"), sep = "\n"))
cat("<<<RPLUGIN<<<")