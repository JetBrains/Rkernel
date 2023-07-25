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

.read_repositories_alias <- NULL
for (package in c("tools", "utils")) {
  package_namespace <- getNamespace(package)
  if (exists(".read_repositories", envir = package_namespace)) {
    .read_repositories_alias <- get(".read_repositories", envir = package_namespace)
    break
  }
}

a <- .read_repositories_alias(p)

cat(">>>RPLUGIN>>>")
writeLines(a[, "URL"])
writeLines("")
writeLines(unlist(unname(getOption("repos"))))
cat("<<<RPLUGIN<<<")