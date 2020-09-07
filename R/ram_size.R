#  Rkernel is an execution kernel for R interpreter
#  Copyright (C) 2020 JetBrains s.r.o.
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
#  along with this program. If not, see <https:#www.gnu.org/licenses/>.


#!/usr/bin/env Rscript

# All this stuff based on benchmarkme::get_ram
remove_white <- function(x) gsub("(^[[:space:]]+|[[:space:]]+$)", "", x)

clean_linux_ram <- function(ram) as.numeric(ram) * 1024
clean_darwin_ram <- function(ram) as.numeric(ram)

clean_solaris_ram <- function(ram) {
  ram <- remove_white(ram)
  to_bytes <- function(value) {
    num <- as.numeric(value[1])
    units <- value[2]
    power <- match(units, c("kB", "MB", "GB", "TB"))
    if (!is.na(power)) return(num * 1000^power)
    power <- match(units, c("Kilobytes", "Megabytes", "Gigabytes", "Terabytes"))
    if (!is.na(power)) return(num * 1000^power)
    num
  }
  to_bytes(unlist(strsplit(ram, " "))[3:4])
}

clean_win_ram <- function(ram) {
  ram <- remove_white(ram)
  ram <- ram[nchar(ram) > 0]
  sum(as.numeric(ram))
}

clean_ram <- function(ram, os) {
  if (length(ram) > 1 || is.na(ram)) return(NA)
  if (length(grep("^linux", os))) {
    clean_ram <- clean_linux_ram(ram)
  } else if (length(grep("^darwin", os))) {
    clean_ram <- clean_darwin_ram(ram)
  } else if (length(grep("^solaris", os))) {
    clean_ram <- clean_solaris_ram(ram)
  } else {
    clean_ram <- clean_win_ram(ram)
  }
  unname(clean_ram)
}

get_windows_ram <- function() {
  ram <- try(system("grep MemTotal /proc/meminfo", intern = TRUE), silent = TRUE)
  if (class(ram) != "try-error" && length(ram) != 0) {
    ram <- strsplit(ram, " ")[[1]]
    mult <- switch(ram[length(ram)], B = 1, kB = 1024, MB = 1048576)
    ram <- as.numeric(ram[length(ram) - 1])
    ram_size <- ram * mult
  } else {
    ram_size <- system("wmic MemoryChip get Capacity", intern = TRUE)[-1]
  }
  return(ram_size)
}

system_ram <- function(os) {
  if (length(grep("^linux", os))) {
    cmd <- "awk \'/MemTotal/ {print $2}\' /proc/meminfo"
    ram <- system(cmd, intern = TRUE)
  } else if (length(grep("^darwin", os))) {
    ram <- substring(system("sysctl hw.memsize", intern = TRUE), 13)
  } else if (length(grep("^solaris", os))) {
    cmd <- "prtconf | grep Memory"
    ram <- system(cmd, intern = TRUE)
  } else {
    ram <- get_windows_ram()
  }
  ram
}

get_ram <- function() {
  os <- R.version$os
  tryCatch({
    ram <- system_ram(os)
    if (length(ram) == 0) {
      NA
    } else {
      cleaned_ram <- clean_ram(ram, os)
      if (length(cleaned_ram) == 0) NA else cleaned_ram
    }
  }, error = function(ignore) NA)
}

# Returns using a strange basis, so that we don't have to worry about inaccuracy. The normal basis will show
# a slightly smaller value than the value we are interested in. Like 0.95 GB instead of 1 GB.
ram <- get_ram() / (1000 ^ 3)

cat(">>>RPLUGIN>>>")
cat(ram)
cat("<<<RPLUGIN<<<")