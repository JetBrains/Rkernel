#!/usr/bin/env Rscript

args = commandArgs(TRUE)

if (length(args) != 0) {
  warning("Usage: installed_packages.R")
  quit(save = "no", status = 1, runLast = FALSE)
}

versions <- as.data.frame(installed.packages()[, c("Package", "Version", "Priority", "LibPath")])
description <- data.frame("Title" = I(lapply(versions[, "Package"], function(x) packageDescription(x, fields = "Title"))),
                         "URL" = I(lapply(versions[, "Package"], function(x) packageDescription(x, fields = "URL"))))
packages <- cbind(versions, description)

cat(">>>RPLUGIN>>>")
with(packages, cat(paste(LibPath, Package, Version, Priority, Title, URL, sep = "^^^JETBRAINS_RPLUGIN^^^"), sep = "!!!JETBRAINS_RPLUGIN!!!"))
cat("<<<RPLUGIN<<<")
