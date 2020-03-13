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


assign(".jetbrains", new.env(), envir = baseenv())

# Set hook to record previous plot before new vanilla graphics are created
setHook(hookName = "before.plot.new",
        value = function() {
          .Call(".jetbrains_ther_device_record", FALSE)
        },
        action = "append")

# Set hook to record previous plot before new ggplot2 graphics are created
setHook(hookName = "before.grid.newpage",
        value = function() {
          .Call(".jetbrains_ther_device_record", TRUE)  # Pass TRUE to indicate it was triggered by ggplot2
        },
        action = "append")

# Force interpreter to restart custom graphics device
options(device = function() {
  .Call(".jetbrains_ther_device_restart")
})

# Some packages might be not available as binaries.
# The default behaviour of interpreter in such a case
# is to ask user whether he wants to install it from source instead.
# This is not desirable that's why interpreter will be forced to install packages from source
# **when necessary** without user permission
options(install.packages.compile.from.source = "always")

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

# Fallback values
.jetbrains$externalImageDir <- "."
.jetbrains$externalImageCounter <- 0

.jetbrains$getNextExternalImagePath <- function(path) {
  snapshot.count <- .Call(".jetbrains_ther_device_snapshot_count")
  if (is.null(snapshot.count)) {
    snapshot.count <- 0
  }
  .jetbrains$externalImageCounter <- .jetbrains$externalImageCounter + 1  # Also ensure it's > 0
  base.name <- paste0("image_", snapshot.count - 1, "_", .jetbrains$externalImageCounter, ".", tools::file_ext(path))
  file.path(.jetbrains$externalImageDir, base.name)
}

.jetbrains$runBeforeChunk <- function(report.text, chunk.text, cache.dir, width, height, resolution) {
  .rs.evaluateRmdParams(report.text)
  opts <- .rs.evaluateChunkOptions(chunk.text)
  data.dir <- file.path(cache.dir, "data")
  html.lib.dir <- file.path(cache.dir, "lib")
  image.dir <- file.path(cache.dir,  "images")
  external.image.dir <- file.path(cache.dir, "external-images")
  dir.create(cache.dir, recursive = TRUE, showWarnings = FALSE)
  dir.create(image.dir, showWarnings = FALSE)
  dir.create(external.image.dir, showWarnings = FALSE)
  dir.create(html.lib.dir, showWarnings = FALSE)
  dir.create(data.dir, showWarnings = FALSE)

  .jetbrains$externalImageDir <- external.image.dir
  .jetbrains$externalImageCounter <- 0
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
}

.jetbrains$findAllNamedArguments <- function(x) {
  ignoreErrors <- function(expr) {
    as.list(tryCatch(expr, error = function(e) { }))
  }

  defenv <- if (!is.na(w <- .knownS3Generics[x])) {
    asNamespace(w)
  } else {
    genfun <- get(x, mode = "function")
    if (.isMethodsDispatchOn() && methods::is(genfun, "genericFunction")) {
      genfun <- methods::finalDefaultMethod(genfun@default)
    }
    if (typeof(genfun) == "closure") environment(genfun)
    else .BaseNamespaceEnv
  }
  s3_table <- get(".__S3MethodsTable__.", envir = defenv)

  getS3Names <- function(row) {
    functionName <- row[["functionName"]]
    from <- row[["from"]]
    envir <- tryCatch(as.environment(from), error = function(e) NULL)
    if (startsWith(from, "registered S3method")) {
      envir <- s3_table
    }
    if (is.null(envir)) {
      envir <- tryCatch(as.environment(paste0("package:", from)), error = function(e) .GlobalEnv)
    }
    names(formals(get(functionName, mode = "function", envir = envir)))
  }

  s4 <- ignoreErrors(names(formals(getMethod(x))))
  s3Info <- attr(.S3methods(x), "info")
  s3Info[, "functionName"] <- rownames(s3Info)
  s3 <- ignoreErrors(apply(s3Info[, c("functionName", "from")], 1, getS3Names))
  unique(unlist(c(s4, s3, names(formals(x)))))
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

.jetbrains$saveRecordedPlotToFile <- function(snapshot, output.path) {
  .jetbrains.recorded.snapshot <- snapshot
  save(.jetbrains.recorded.snapshot, file = output.path)
}

.jetbrains$replayPlotFromFile <- function(input.path) {
  load(input.path)
  plot <- .jetbrains.recorded.snapshot

  # restore native symbols for R >= 3.0
  rVersion <- getRversion()
  if (rVersion >= "3.0") {
    for (i in 1:length(plot[[1]])) {
      # get the symbol then test if it's a native symbol
      symbol <- plot[[1]][[i]][[2]][[1]]
      if ("NativeSymbolInfo" %in% class(symbol)) {
        # determine the dll that the symbol lives in
        name = if (!is.null(symbol$package)) symbol$package[["name"]] else symbol$dll[["name"]]
        pkgDLL <- getLoadedDLLs()[[name]]

        # reconstruct the native symbol and assign it into the plot
        nativeSymbol <- getNativeSymbolInfo(name = symbol$name, PACKAGE = pkgDLL, withRegistrationInfo = TRUE);
        plot[[1]][[i]][[2]][[1]] <- nativeSymbol;
      }
    }
  }

  # Replay obtained plot
  suppressWarnings(grDevices::replayPlot(plot, reloadPkgs=TRUE))
}

.jetbrains$dropRecordedSnapshots <- function(device.number, from, to) {
  for (i in from:to) {
    name <- paste0("recordedSnapshot_", device.number, "_", i)
    if (exists(name, envir = .jetbrains)) {
      assign(name, NULL, .jetbrains)
    }
  }
}

.jetbrains$unloadLibrary <- function(package.name, with.dynamic.library) {
  resource.name <- paste0("package:", package.name)
  detach(resource.name, unload = TRUE, character.only = TRUE)
  if (with.dynamic.library) {
    .jetbrains$unloadDynamicLibrary(package.name)
  }
}

.jetbrains$unloadDynamicLibrary <- function(package.name) {
  if (.jetbrains$isDynamicLibraryLoaded(package.name)) {
    pd.file <- attr(packageDescription(package.name), "file")
    lib.path <- sub("/Meta.*", "", pd.file)
    library.dynam.unload(package.name, libpath = lib.path)
  }
}

.jetbrains$isDynamicLibraryLoaded <- function(package.name) {
  for (lib in .dynLibs()) {
    name <- lib[[1]]
    if (name == package.name) {
      return(TRUE)
    }
  }
  FALSE
}

local({
  env <- as.environment("package:utils")
  unlockBinding("View", env)
  env$View <- function(x, title = paste(deparse(substitute(x)), collapse = " "))
    invisible(.Call(".jetbrains_View", x, title))
  lockBinding("View", env)
})

if (.Platform$OS.type == "unix" && !("UTF-8" %in% localeToCharset(Sys.getlocale("LC_CTYPE")))) {
  if (grepl("^darwin", R.version$os)) {
    Sys.setlocale("LC_CTYPE", "UTF-8")
  } else {
    Sys.setlocale("LC_CTYPE", "C.UTF-8")
  }
}

options(warn = 1)
options(demo.ask = TRUE);
assign(".Last.sys", function() .Call(".jetbrains_quitRWrapper"), envir = baseenv())