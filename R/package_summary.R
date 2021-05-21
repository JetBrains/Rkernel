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


#!/usr/bin/env Rscript

# Some packages can assign functions with name equals to used in script
myCat <- base::cat
myClass <- base::class
myLength <- base::length
myNames <- base::names
myPaste <- base::paste
myPaste0 <- base::paste0
myGet <- base::get

arguments <- commandArgs(TRUE)

if (myLength(arguments) < 2) {
  warning("Usage: package_summary.R <extraNamedArguments_path> <package_names>...")
  quit(save = "no", status = 1, runLast = FALSE)
}

assign(".jetbrains", new.env(), envir = globalenv())
source(arguments[[1]]) # extraNamedArguments.R

attachAndLoadNamespace <- function(pName) {
  suppressMessages(suppressWarnings(base::library(pName, character.only = T))) # Attach necessary for extraNamedArgs
  loadNamespace(pName)
}

processPackage <- function(pName) {
  namespace <- tryCatch(attachAndLoadNamespace(pName), error = function(e) {
    myCat(">>>RPLUGIN>>>")
    myCat("intellij-cannot-load-package")
    myCat(gettext(e))
    myCat("<<<RPLUGIN<<<")
    NULL
  })
  if (is.null(namespace)) return()

  myCat(">>>RPLUGIN>>>")
  pPriority <- packageDescription(pName)$Priority  # Seems to be more efficient than looking through all installed packages
  pPriority <- if (is.null(pPriority)) {
    NA
  } else {
    toupper(pPriority)
  }
  myCat(pPriority)


  exportedSymbols <- getNamespaceExports(namespace)
  allSymbols <- base::unique(c(base::ls(envir = namespace, all.names = TRUE), exportedSymbols))

  #' some symbols are defined as functions in base.R but our parser does not like it.
  ignoreList <- if (pName == "base") c("for", "function", "if", "repeat", "while") else NULL

  symbolSignature <- function(symbol, types, isExported, spec = NULL) {
    str <- c(symbol, isExported, myLength(types))
    str <- c(str, types, spec)
    myPaste(str, collapse = '\001')
  }

  cache <- new.env()
  for (symbol in allSymbols) {
    if (symbol %in% ignoreList) next
    obj <- myGet(symbol, envir = namespace)

    types <- myClass(obj)
    spec <- NULL
    isExported <- symbol %in% exportedSymbols
    if ("function" %in% types) {
      description <- args(obj)
      if (myLength(description) > 0) {
        description <- deparse(description)
        spec <- myPaste(description[seq_len(myLength(description) - 1)], collapse = "")
        # See extraNamedArguments.R#.jetbrains$findExtraNamedArgs for details
        if (isExported) {
          extraNamedArgs <- tryCatch(.jetbrains$findExtraNamedArgs(symbol, 2, package = pName, cache = cache), error = function(e) {
            base::message(e)
          })
          if (myLength(extraNamedArgs) > 0) {
            argNames <- NULL # Names of arguments which can be passed directly to `...`
            funNames <- NULL # Names of arguments which is function whose arguments can also be passed to `...`
            lapply(extraNamedArgs, function(x) {
              if (!x[[2]]) {
                argNames <<- c(argNames, x[[1]])
              }
              else {
                funNames <<- c(funNames, x[[1]])
              }
            })
            spec <- c(spec, myPaste(argNames, collapse = ";"), myPaste(funNames, collapse = ";"))
          }
        }
      }
    }
    myCat("\n")
    myCat(symbolSignature(symbol, types, isExported, spec))
  }

  ##
  ## Also export datasets from package into skelekton
  ##

  # http://stackoverflow.com/questions/27709936/how-to-get-the-list-of-data-sets-in-a-particular-package
  if (!(pName %in% c("base", "stats", "backports"))) {
    dsets <- as.data.frame(utils::data(package = pName)$result)

    ## this fails for packages like 'fields' that export data as symbol and data
    ## .. thus we rather just remove such duplicates here
    dsets <- subset(dsets, !(Item %in% allSymbols))


    dsets <- subset(dsets, !(0:nrow(dsets) %in% base::grep("(", as.character(dsets$Item), fixed = TRUE))[-1])

    for (symbol in base::with(dsets, as.character(Item))) {
      types <- tryCatch(myClass(base::eval(parse(text = myPaste0(pName, "::`", symbol, "`")))), error = function(e) {})
      if (myLength(types) != 0) {
        myCat("\n")
        myCat(symbolSignature(symbol, types, TRUE))
      }
    }
  }

  classTable <- methods:::.classTable
  # class_name types... <slotName, slotType>... superClasses... isVirtual
  lapply(myNames(classTable), function(className) {
    class <- classTable[[className]]
    if (inherits(class, "classRepresentation") && class@package == pName) {
      spec <- NULL
      slots <- sapply(myNames(class@slots), function(s) c(s, class@slots[[s]]))
      spec <- c(spec, myLength(slots), unlist(slots))
      superClasses <- sapply(class@contains, function(y) y@superClass)
      spec <- c(spec, myLength(superClasses), unlist(superClasses))
      myCat("\n")
      myCat(symbolSignature(className, myClass(class), TRUE, c(spec, class@virtual)))
    }
  })
  myCat("<<<RPLUGIN<<<")
}

for (i in seq.int(2, myLength(arguments))) {
  tryCatch({
    processPackage(arguments[[i]])
    flush(stdout())
  }, error = function(e) {
    myCat(gettext(e))
    myCat("<<<RPLUGIN<<<")
  })
}