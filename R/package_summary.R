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

arguments = commandArgs(TRUE)

if (length(arguments) != 2) {
  warning("Usage: package_summary.R <package_name> <extraNamedArguments_path>")
  quit(save = "no", status = 1, runLast = FALSE)
}

attachAndLoadNamespace <- function(pName) {
  suppressMessages(suppressWarnings(library(pName, character.only = T))) # Attach necessary for extraNamedArgs
  loadNamespace(pName)
}

pName = arguments[[1]]

cat(">>>RPLUGIN>>>")
namespace = tryCatch(attachAndLoadNamespace(pName), error = function(e) {
  cat("intellij-cannot-load-package")
  message(e)
  cat("<<<RPLUGIN<<<")
  quit(save = "no", status = 1, runLast = FALSE)
})


pPriority = packageDescription(pName) $ Priority  # Seems to be more efficient than looking through all installed packages
pPriority = if (is.null(pPriority)) {
    NA
} else {
    toupper(pPriority)
}
cat(pPriority)


exportedSymbols = getNamespaceExports(namespace)
allSymbols = unique(c(ls(envir = namespace, all.names = TRUE), exportedSymbols))
#exportedSymbols = allSymbols = getNamespaceExports(namespace)

#' some symbols are defined as functions in base.R but our parser does not like it.
ignoreList = c("for", "function", "if", "repeat", "while")

symbolSignature <- function(symbol, types, isExported, spec = c()) {
  str = c(symbol, isExported, length(types))
  str = c(str, types, spec)
  paste(str, collapse='\001')
}

assign(".jetbrains", new.env(), envir = baseenv())
source(arguments[[2]]) # extraNamedArguments.R
cache <- new.env()

for (symbol in allSymbols) {
  if (symbol %in% ignoreList) next
  obj = base::get(symbol, envir = namespace)

  types = class(obj)
  spec <- NULL
  if ("function" %in% types) {
    description <- args(obj)
    if (length(description) > 0) {
      description <- deparse(description)
      spec <- paste(description[seq_len(length(description) - 1)], collapse = "")
      # See extraNamedArguments.R#.jetbrains$findExtraNamedArgs for details
      extraNamedArgs <- tryCatch(.jetbrains$findExtraNamedArgs(symbol, 2, cache), error = function(e) {
        write(geterrmessage(), file = stderr())
      })
      if (length(extraNamedArgs) > 0) {
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
        spec <- c(spec, paste(argNames, collapse = ";"), paste(funNames, collapse = ";"))
      }
    }
  }
  isExported = symbol %in% exportedSymbols
  cat("\n")
  cat(symbolSignature(symbol, types, isExported, spec))
}

##
## Also export datasets from package into skelekton
##

# http://stackoverflow.com/questions/27709936/how-to-get-the-list-of-data-sets-in-a-particular-package
if (!(pName %in% c("base", "stats", "backports"))) {
  dsets = as.data.frame(data(package = pName)$result)

## this fails for packages like 'fields' that export data as symbol and data
## .. thus we rather just remove such duplicates here
  dsets = subset(dsets, !(Item %in% allSymbols))


  dsets = subset(dsets, !(0:nrow(dsets) %in% grep("(", as.character(dsets$Item), fixed = TRUE))[-1])

  for(symbol in with(dsets, as.character(Item))) {
    types = tryCatch(class(eval(parse(text=paste(pName, "::`", symbol, "`", sep="")))), error = function(e) c())
    if (length(types) != 0) {
      cat("\n")
      cat(symbolSignature(symbol, types, TRUE))
    }
  }
}
cat("<<<RPLUGIN<<<")