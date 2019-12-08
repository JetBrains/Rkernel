#!/usr/bin/env Rscript

args = commandArgs(TRUE)

if (length(args) < 1) {
  warning("Usage: available_packages.R <repo_url>*")
  quit(save = "no", status = 1, runLast = FALSE)
}

remove_newlines <- function(s) {
  gsub("\r?\n|\r", " ", s)
}

repo_urls <- args
old_repos <- getOption("repos")
options(repos = repo_urls)
p <- available.packages()[, c("Package", "Repository", "Version", "Depends")]
p <- as.data.frame(p)
p$Depends <- sapply(p$Depends, remove_newlines)
cat(">>>RPLUGIN>>>")
with(p, cat(paste(paste(Package, Repository, Version, sep = " "), Depends, sep = "\t"), sep = "\n"))
cat("<<<RPLUGIN<<<")
options(repos = old_repos)
