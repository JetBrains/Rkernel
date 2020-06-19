#!/usr/bin/env Rscript

args = commandArgs(TRUE)

if (length(args) == 0) {
  warning("Usage: get_env.R <env_name> [--normalize-path]")
  quit(save = "no", status = 1, runLast = FALSE)
}

env_name <- args[1]
s <- Sys.getenv(env_name)
if (length(args) > 1 && "--normalize-path" %in% args[2:length(args)]) {
  s <- normalizePath(s)
}
cat(">>>RPLUGIN>>>")
cat(s, sep = "\n")
cat("<<<RPLUGIN<<<")