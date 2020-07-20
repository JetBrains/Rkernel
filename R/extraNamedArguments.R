# If you have made significant changes here, it may be worth looking at
# rplugin/test/extraNamedArgsPerformanceTest.R to check performance

.jetbrains$funcArgsInternal <<- list2env(list(
  lapply = "FUN",
  sapply = "FUN",
  vapply = "FUN",
  rapply = "f"))

.jetbrains$namedArgsInternal <<- list2env(list(
  tryCatch = c("error", "warning"),
  withCallingHandlers = c("error", "warning"),
  write.csv = c("x", "file", "quote", "eol", "na", "row.names", "fileEncoding"), # Some params of write.table ignored
  write.csv2 = c("x", "file", "quote", "eol", "na", "row.names", "fileEncoding")))

.jetbrains$safeEq <<- function(x, y) {
  res <- x == y
  if (!is.logical(res) || length(res) == 0) FALSE
  else res
}

.jetbrains$safeSapply <<- function(list, fun, ..., dropNull = TRUE) {
  if (length(list) == 0) return(NULL)
  emptyHandler <- function(ignore) { }
  result <- sapply(list, function(e, ...) tryCatch(fun(e, ...), error = emptyHandler, warning = emptyHandler), ...)
  if (dropNull) {
    result[sapply(result, function(x) !is.null(x))]
  }
  else {
    result
  }
}

.jetbrains$safePredicateAny <<- function(list, predicate, ...) {
  any(.jetbrains$safeSapply(list, predicate, ..., dropNull = FALSE), na.rm = TRUE)
}

.jetbrains$isFunctionNode <<- function(node) {
  class(node) == "call" && node[[1]] == "function"
}

.jetbrains$safeFormalArgs <<- function(l, package = NULL) {
  env_formals <- function (fun, envir) {
    if (is.character(fun)) fun <- get(fun, mode = "function", envir = envir)
    .Internal(formals(fun))
  }
  .jetbrains$safeSapply(l, function(def) {
    env <- tryCatch(asNamespace(package), error = function(e) { parent.frame() })
    names(env_formals(def, envir = env))
  })
}

.jetbrains$bindArgs <<- function(params, call) {
  args <- call[seq(2, length(call))]
  args <- args[sapply(args, function(x) !.jetbrains$safeEq(x, "..."))]
  res <- new.env()
  argsNames <- names(args)
  namedArguments <- args[argsNames != ""]
  namedArgNames <- argsNames[argsNames != ""]
  for (i in seq_len(length(namedArgNames))) {
    assign(namedArgNames[[i]], namedArguments[[i]], res)
  }

  paramCnt <- length(params)
  paramInd <- 1
  otherArgs <- if (is.null(argsNames)) args else args[argsNames == ""]
  sapply(otherArgs, function(x) {
    while (paramInd <= paramCnt && exists(params[[paramInd]], envir = res, inherits = FALSE)) {
      paramInd <- paramInd + 1
    }
    if (paramInd > paramCnt) return()
    paramName <- params[[paramInd]]
    if (paramName != "...") assign(paramName, x, res)
  })
  res
}

.jetbrains$findNodeExtraNamedArgs <<- function(node, functionParams, depth, package, cache, recStack) {
  if (!is.recursive(node) || depth <= 0) return()
  if (class(node) != "call" || length(node) < 2) {
    # Not a function/function call. Or argument list is empty.
    Reduce(c, lapply(node, .jetbrains$findNodeExtraNamedArgs, functionParams = functionParams,
                     depth = depth, package = package, cache = cache, recStack = recStack))
  }
  else {
    if (.jetbrains$isFunctionNode(node)) {
      # Function definition
      paramsNames <- names(node[[2]])
      if (.jetbrains$safePredicateAny(paramsNames, .jetbrains$safeEq, x = "...")) {
        # Definition with ... param -> no need to collect info from body
        NULL
      }
      else {
        # Function definition without ... -> collect info from body
        .jetbrains$findNodeExtraNamedArgs(node[[3]], paramsNames, depth - 1, package = package, cache = cache, recStack = recStack)
      }
    }
    else {
      res <- Reduce(c, lapply(node, .jetbrains$findNodeExtraNamedArgs, functionParams = functionParams,
                              depth = depth, package = package, cache = cache, recStack = recStack))
      funName <- if (is.symbol(node[[1]])) {
        node[[1]]
      } else if (class(node[[1]]) == "call") {
        ident <- node[[1]]
        if (ident[[1]] == "::" && is.symbol(ident[[2]]) && is.symbol(ident[[3]])) {
          package <- ident[[2]]
          ident[[3]]
        }
        else NULL
      } else {
        NULL
      }
      if (!is.null(funName) && .jetbrains$safePredicateAny(node, .jetbrains$safeEq, x = "...")) {
        # Function call with ... agrument
        callName <- as.character(funName)
        if (.jetbrains$safePredicateAny(functionParams, .jetbrains$safeEq, x = callName)) {
          # Depends on parameters
          res <- c(res, list(list(callName, TRUE)))
        }
        else {
          args <- .jetbrains$safeFormalArgs(callName, package)
          binded <- .jetbrains$bindArgs(args, node)

          filterArgs <- function(argList) {
            if (length(argList) == 0) return(NULL)
            argList[sapply(argList, function(x) {
              !exists(x, envir = binded, inherits = FALSE) &&
                x != "..." &&
                !.jetbrains$safePredicateAny(functionParams, .jetbrains$safeEq, x = x)
            })]
          }

          externalArgs <- NULL
          funExtraArgs <- lapply(.jetbrains$findExtraNamedArgs(callName, depth - 1, package = package, cache = cache, recStack = recStack), function(x) {
            name <- x[[1]]
            isFunForCompletion <- x[[2]]
            column <- list(name, isFunForCompletion)
            if (isFunForCompletion) {
              val <- binded[[name]]
              if (!is.null(val)) {
                if (class(val) == "name") {
                  charVal <- as.character(val)
                  if (.jetbrains$safePredicateAny(functionParams, .jetbrains$safeEq, x = charVal)) {
                    list(as.character(val), TRUE)
                  }
                  else {
                    externalArgs <<- c(res, lapply(filterArgs(.jetbrains$safeFormalArgs(charVal, package)), function(x) list(x, F)))
                    NULL
                  }
                }
                else NULL
              }
              else column
            }
            else {
              if (exists(name, envir = binded, inherits = FALSE)
                || .jetbrains$safePredicateAny(functionParams, .jetbrains$safeEq, x = name)) NULL else column
            }
          })
          if (length(funExtraArgs) > 0) {
            funExtraArgs <- funExtraArgs[sapply(funExtraArgs, function(x) !is.null(x))]
          }
          res <- c(res, funExtraArgs, externalArgs, lapply(filterArgs(args), function(x) list(x, F)))
        }
      }
      res
    }
  }
}

.jetbrains$addArgsFromInternal <<- function(x, result) {
  if (is.character(x) && length(x) == 1) {

    add <- function(envir, val) {
      if (exists(x, envir = envir, inherits = FALSE)) {
        result <<- c(result, lapply(envir[[x]], function(x) list(x, val)))
      }
    }

    add(.jetbrains$funcArgsInternal, TRUE)
    add(.jetbrains$namedArgsInternal, FALSE)
  }
  result
}

#' @title Extra named arguments
#' @returns List of pairs of type <character(1), logical(1)>. If second element of pair is FALSE, then
#' first element is name of argument which can be passed directly to `...`. Otherwise
#' first element is name of argument which is function whose arguments can also be passed to `...`.
.jetbrains$findExtraNamedArgs <<- function(x, depth, package = NULL, cache = new.env(), recStack = NULL) {
  if (depth <= 0) return(NULL)
  if (is.character(x) && length(x) == 1) {
    if (.jetbrains$safePredicateAny(recStack, .jetbrains$safeEq, x = x)) return()
    if (exists(x, envir = cache, inherits = FALSE)) {
      val <- cache[[x]]
      charDepth <- as.character(depth)
      if (hasName(val, charDepth)) return(val[[charDepth]])
    }
  }
  if (depth <= 0) return()
  params <- NULL
  body <- NULL
  str2lang <- function(s) parse(text = s, keep.source = FALSE)[[1]]
  tryCatch({
      fun <- tryCatch(get(x, mode = "function", envir = asNamespace(package), inherits = FALSE), error = function(e) {
        get(x, mode = "function", envir = parent.frame())
      })
      body <- base::body(fun)
      params <- formalArgs(fun)
  }, error = function(e) {
    node <- tryCatch(str2lang(x), error = function(e) { }, warning = function(w) { })
    if (.jetbrains$isFunctionNode(node)) {
      params <<- names(node[[2]])
      body <<- node[[3]]
    }
  }, warning = function(w) { })
  if (!is.null(body) && .jetbrains$safePredicateAny(params, .jetbrains$safeEq, x = "...")) {
    res <- unique(.jetbrains$addArgsFromInternal(x, .jetbrains$findNodeExtraNamedArgs(body, params, depth, package, cache, c(recStack, x))))
    if (is.character(x) && length(x) == 1) {
      if (!exists(x, envir = cache, inherits = FALSE)) assign(x, list(), cache)
      listRes <- list(res)
      names(listRes) <- as.character(depth)
      assign(x, c(cache[[x]], listRes), cache)
    }
    res
  }
}
