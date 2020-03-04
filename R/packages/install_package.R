#!/usr/bin/env Rscript

args = commandArgs(TRUE)
devtools::install(reload = FALSE, upgrade = "never", build = FALSE, args = args)
