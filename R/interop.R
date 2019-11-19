#  Rkernel is an execution kernel for R interpreter
#  Copyright (C) 2019 JetBrains s.r.o.
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https:#www.gnu.org/licenses/>.


assign(".jetbrains", new.env(), baseenv())

# Set hook before new vanilla graphics are created
setHook(hookName = "before.plot.new",
        value = function() {
          .Call(".jetbrains_ther_device_dump")
        },
        action = "append")

# Set hook before new ggplot2 graphics are created
setHook(hookName = "before.grid.newpage",
        value = function() {
          .Call(".jetbrains_ther_device_dump")
        },
        action = "append")

.jetbrains$init <- function(rsession.path, project.dir) {
 current.wd <- getwd()
  tryCatch({
    tools.path <- file.path(rsession.path, "Tools.R")
    modules.path <- file.path(rsession.path, "modules")
    setwd(modules.path)
    source(tools.path)
    sapply(Filter(function(s) s != "SessionCompileAttributes.R" & s != "SessionPlots.R", list.files(modules.path, pattern = ".*\\.r$", ignore.case = TRUE)), function(x) { source(file.path(modules.path, x)) } )
    .rs.getProjectDirectory <<- function() project.dir
    options(BuildTools.Check = NULL)
  }, finally = {
    setwd(current.wd)
  })
}

.jetbrains$updatePackageEvents <- function() {
  packageNames <- base::list.dirs(.libPaths(), full.names = FALSE, recursive = FALSE)
  sapply(packageNames, function(name) {
    if (!(name %in% .rs.jbHookedPackages)) {
      loadEventName = packageEvent(name, "onLoad")
      onPackageLoaded <- function(name, ...) {
        .rs.reattachS3Overrides()
      }
      setHook(loadEventName, onPackageLoaded, action = "append")
      .rs.setVar("jbHookedPackages", append(.rs.jbHookedPackages, name))
    }
  })
}

.jetbrains$setNotebookGraphicsOption <- function(image.dir, width, height, resolution) {
  file.name <- file.path(image.dir, "%d.png")
  options(device = function() {
    png(file.name, width, height, res = resolution)
    dev.control(displaylist = "enable")
    par(mar = c(5.1, 4.1, 2.1, 2.1))
  })
}

.jetbrains$runBeforeChunk <- function(report.text, chunk.text, cache.dir, width, height, resolution) {
  .rs.evaluateRmdParams(report.text)
  opts <- .rs.evaluateChunkOptions(chunk.text)
  data.dir <- file.path(cache.dir, "data")
  html.lib.dir <- file.path(cache.dir, "lib")
  image.dir <- file.path(cache.dir,  "images")
  dir.create(cache.dir, recursive = TRUE, showWarnings = FALSE)
  dir.create(image.dir, showWarnings = FALSE)
  dir.create(html.lib.dir, showWarnings = FALSE)
  dir.create(data.dir, showWarnings = FALSE)

  while (grDevices::dev.cur() != 1) {
    grDevices::dev.off()
  }
  .jetbrains$setNotebookGraphicsOption(image.dir, width, height, resolution)
  .rs.initHtmlCapture(cache.dir, html.lib.dir, opts)
  .rs.initDataCapture(data.dir, opts)
  if (!.rs.hasVar("jbHookedPackages")) {
    .rs.setVar("jbHookedPackages", character())
  }
  .jetbrains$updatePackageEvents()  # Note: supposed to be useless when packages are getting installed within chunk but for my machine it's OK
}

.jetbrains$runAfterChunk <- function() {
  .rs.releaseHtmlCapture()
  .rs.releaseDataCapture()
  if (grDevices::dev.cur() != 1) {
    grDevices::dev.off()
  }
}

.jetbrains$findAllNamedArguments <- function(x) {
  ignoreErrors <- function(expr) {
    as.list(tryCatch(expr, error = function(e) {}))
  }

  getS3Names <- function(row) {
    functionName <- row[["functionName"]]
    from <- row[["from"]]
    envir <- tryCatch(as.environment(from), error = function(e) NULL)
    if (startsWith(from, "registered S3method")) {
      envir <- get(".__S3MethodsTable__.")
    }
    if (is.null(envir)) {
      envir <- tryCatch(as.environment(paste("package:", from)), error = function(e) .GlobalEnv)
    }
    names(formals(get(functionName, mode = "function", envir = envir)))
  }

  s4 <- ignoreErrors(names(formals(getMethod(x))))
  s3Info <- attr(.S3methods(x), "info")
  s3Info[, "functionName"] <- rownames(s3Info)
  s3 <- ignoreErrors(apply(s3Info[, c("functionName", "from")], 1, getS3Names))
  unique(unlist(c(s4, s3)))
}

.jetbrains$printAllPackagesToFile <- function(repo.urls, output.path) {
  remove.newlines <- function(s) {
    gsub("\r?\n|\r", " ", s)
  }

  old.repos <- getOption("repos")
  options(repos = repo.urls)
  sink(output.path)
  p <- available.packages()[, c("Package", "Repository", "Version", "Depends")]
  p <- as.data.frame(p)
  p$Depends <- sapply(p$Depends, remove.newlines)
  with(p, cat(paste(paste(Package, Repository, Version, sep = " "), Depends, sep = "\t"), sep = "\n"))
  sink()
  options(repos = old.repos)
}

.jetbrains$getDefaultRepositories <- function() {
  p <- file.path(Sys.getenv("HOME"), ".R", "repositories")
  if (!file.exists(p))
    p <- file.path(R.home("etc"), "repositories")
  a <- tools:::.read_repositories(p)
  a[, "URL"]
}

.jetbrains$printCranMirrorsToFile <- function(output.path) {
  sink(output.path)
  mirrors <- getCRANmirrors()[, c('Name', 'URL')]
  with(mirrors, cat(paste(Name, URL, sep = "\t"), sep = "\n"))
  sink()
}

.jetbrains$printInstalledPackagesToFile <- function(output.path) {
  sink(output.path)
  versions <- as.data.frame(installed.packages()[, c("Package", "Version", "Priority", "LibPath")])
  with(versions, cat(paste(LibPath, Package, Version, Priority, sep = "\t"), sep = "\n"))
  sink()
}

local({
  env = as.environment("package:utils")
  unlockBinding("View", env)
  env$View = function(x, title = paste(deparse(substitute(x)), collapse = " "))
    invisible(.Call(".jetbrains_View", x, title))
  lockBinding("View", env)
})
