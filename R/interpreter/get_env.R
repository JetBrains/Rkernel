#!/usr/bin/env Rscript

args = commandArgs(TRUE)

if (length(args) != 1) {
  warning("Usage: get_env.R <env_name>")
  quit(save = "no", status = 1, runLast = FALSE)
}

env_name <- args[1]
cat(">>>RPLUGIN>>>")
cat(Sys.getenv(env_name), sep = "\n")
cat("<<<RPLUGIN<<<")