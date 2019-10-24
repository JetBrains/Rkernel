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

args = commandArgs(TRUE)

if (length(args) != 2) {
    warning("Usage: extract_symbol.R <package_name> <symbol_name>")
    quit(save = "no", status = 1, runLast = FALSE)
}

pName = args[1]
symbol = args[2]

namespace = loadNamespace(pName)

is_identifier = function(str) {
    return(grepl("^([[:alpha:]]|\\.)([[:alpha:]]|[[:digit:]]|_|\\.)*$", str) == TRUE)
}


pckgPrefixed = paste("package", pName, sep=":")

exports =  getNamespaceExports(namespace)
functions = ls(exports, envir = namespace)

#' some symbols are defined as functions in base.R but our parser does not like it.
ignoreList = c("for", "function", "if", "repeat", "while")

# http://stackoverflow.com/questions/26174703/get-namespace-of-function
detect_declaring_ns = function(symbol){
    resolveResult = getAnywhere(symbol)
    origin = resolveResult$where
    origin = origin[grepl("namespace:", origin)][1]
    unlist(strsplit(origin, ":"))[2]
}

get_text_of_object = function(tmpFile, obj, use_dput){
    sink(tmpFile)
    if (use_dput) {
        dput(obj)
    }else {
        # there is no print for certain methods like R.utils::symbol that's we we need to wrap it with tryCatch
        # print(obj)
        tryCatch(print(obj), error = function(e) "")
    }
    sink()

    ## read the result back into a character vector
    fileObj = file(tmpFile)
    lines = readLines(fileObj)
    close(fileObj)

    lines
}



get_text_of = function(obj){
    tmpFileName = tempfile(pattern = "tmp", tmpdir = tempdir(), fileext = "")

    lines = get_text_of_object(tmpFileName, obj, use_dput = FALSE)

    ## use it if it's a function declaration
    if (grepl('^(function|new)', lines[1])) {
        return(lines)
    }

    ## and fall back to print if not
    # http://stackoverflow.com/questions/31467732/does-r-have-function-startswith-or-endswith-like-python

    raw_text = get_text_of_object(tmpFileName, obj, use_dput = TRUE)
}

# http://stackoverflow.com/questions/2261079/how-to-trim-leading-and-trailing-whitespace-in-r
trim = function (x) gsub("^\\s+|\\s+$", "", x)


quote_non_identifier = function(symbol){
    if (is_identifier(symbol)) {
        symbol
    } else {
        paste0("`", symbol, "`")
    }
}



if (symbol %in% functions && !(symbol %in% ignoreList)) {
    # from http://adv-r.had.co.nz/Environments.html#env-basics
    # The parent of the global environment is the last package that you attached with library() or require().
    obj = base::get(symbol, envir = namespace)
    # if (class(obj) != "function") {
    #     next
    # }


    ## start writing the entry to the skeleton

    cat(quote_non_identifier(symbol))
    cat(" <- ")

    ## handle rexported
    decl_ns = detect_declaring_ns(symbol)
    if (pName != decl_ns) {
        cat(paste0(decl_ns, "::", quote_non_identifier(symbol), " # re-exported from ", decl_ns, " package"))
        cat("\n\n")
        sink()
        quit()
    }

    # process non-function objects
    # TODO instead fo string we could/should write more typed placeholder structure here
    lines = get_text_of(obj)

    if (substring(lines[[1]], 0, 1) == "<") {
        cat("\"", trim(lines[[1]]), "\"", sep = "")
        cat("\n\n")
        sink()
        quit()
    }

    for (line in lines) {
        line = gsub("<pointer: ([A-z0-9]*)>", "pointer(\"\\1\")", line)
        # line = gsub("<S4 object ([A-z0-9]*)>", "(\"\\1\")", line)
        line = gsub("<S4 object of class .*>", "S4_object()", line)

        # fix ellipsis  (...) for which quotes are skipped when printing method body.
        # Potentially this should be rather fixed in the parser
        # Example purr::partial vs https://github.com/hadley/purrr/blob/master/R/partial.R
        # DEBUG line='    args = list(... = quote(expr = ))'
        line = gsub("... = ...", "...", line, fixed = T)
        line = gsub("(... =", "(\"...\" =", line, fixed = T)
        line = gsub(" ... =", " \"...\" =", line, fixed = T)
        line = gsub("<environment>", " \"<environment>\"", line, fixed = T)

        if (grepl("^<environment", line))break
        if (grepl("^<bytecode", line))break

        cat(line, append = TRUE)
        cat("\n", append = TRUE)
    }
    sink()
}
