#!/usr/bin/env Rscript

args = commandArgs(TRUE)

if (length(args) != 0) {
  warning("Usage: default_repositories.R")
  quit(save = "no", status = 1, runLast = FALSE)
}

p <- file.path(Sys.getenv("HOME"), ".R", "repositories")
if (!file.exists(p)) {
  p <- file.path(R.home("etc"), "repositories")
}
a <- tools:::.read_repositories(p)
cat(">>>RPLUGIN>>>")
writeLines(a[, "URL"])
writeLines("")
writeLines(unlist(unname(getOption("repos"))))
cat("<<<RPLUGIN<<<")