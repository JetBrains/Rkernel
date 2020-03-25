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

install_pandoc <- function(library_path) {
  find_pandoc_url <- function(os_suffix, archive_extension) {
    base_url <- "https://github.com/jgm/pandoc/releases"
    page <- readLines(base_url, warn = FALSE)
    pat <- paste0("jgm/pandoc/releases/download/[0-9.]+/pandoc-[0-9.-]+-", os_suffix, ".", archive_extension)
    target_line <- grep(pat, page, value = TRUE)
    m <- regexpr(pat, target_line)
    URLs <- regmatches(target_line, m)
    URL <- head(URLs, 1)
    URL <- paste("https://github.com/", URL, sep="")
    URL
  }

  clear_folder <- function(library_path) {
    lapply(list.files(library_path), function(name) {
      absolute_path <- paste0(library_path, "/", name)
      file.remove(absolute_path)
    })
  }

  download_pandoc <- function(library_path, archive_extension, URL) {
    archive_path <- paste0(library_path, "/pandoc.", archive_extension)
    download.file(URL, destfile = archive_path, mode="wb")
    archive_function <- if (archive_extension == "zip") unzip else untar
    archive_function(archive_path, exdir = library_path)
    file.remove(archive_path)
  }

  remove_nested_folder <- function(library_path, folder_pattern) {
    library_contents <- list.files(library_path, pattern=folder_pattern)
    if (length(library_contents) == 1) {
      folder <- head(library_contents, 1)
      folder_absolute <- paste0(library_path, "/", folder)
      lapply(list.files(folder_absolute), function(name) {
        full_name <- paste0(folder_absolute, "/", name)
        new_name <- paste0(library_path, "/", name)
        file.rename(full_name, new_name)
      })
      file.remove(folder_absolute)
    }
  }

  tune_pandoc <- function(library_path, executable_name) {
    pandoc_executable_path <- paste0(library_path, "/", executable_name)
    Sys.chmod(pandoc_executable_path, mode="0777")
  }

  os_name <- Sys.info()["sysname"]
  os_suffix <- if (os_name == "Linux") {
    "linux"
  } else {
    if (os_name == "Darwin") {
      "macOS"
    } else {
      "windows-x86_64"
    }
  }
  archive_extension <- if (os_name == "Linux") "tar.gz" else "zip"
  executable_name <- if (os_name == "Windows") "pandoc.exe" else "pandoc"
  URL <- find_pandoc_url(os_suffix, archive_extension)
  clear_folder(library_path)
  download_pandoc(library_path, archive_extension, URL)
  remove_nested_folder(library_path, "pandoc*")
  remove_nested_folder(library_path, "bin")
  tune_pandoc(library_path, executable_name)
}

args = commandArgs(TRUE)

if (length(args) != 3 && length(args) != 4) {
    warning(paste0("Usage: render_markdown.R <library_path> <file_path> <render_directory> [result_file]\n",
                   "  Path to output will be saved to [result_file]"))
    quit(save = "no", status = 1, runLast = FALSE)
}

library_path <- args[1]
file_path <- args[2]
knit_root_directory <- args[3]
if (length(args) >= 4) {
  result_file <- args[4]
} else {
  result_file <- NA
}

# RMarkdown looks for Pandoc in PATH and RSTUDIO_PANDOC envs
# We don't want to touch RStudio's envs that's why we'd rather adjust current PATH
old_path <- Sys.getenv("PATH")
path_separator <- if (Sys.info()["sysname"] == "Windows") ";" else ":"
new_path <- paste0(old_path, path_separator, library_path)
Sys.setenv(PATH = new_path)

# First of all, we need to ensure Pandoc is really here (very important)
minimum_pandoc_version <- "1.12.3"  # Note: it's hard coded into code for `rmarkdown::render` (No kidding!)
if (!rmarkdown::pandoc_available(minimum_pandoc_version)) {
  install_pandoc(library_path)
}

runtime <-rmarkdown::yaml_front_matter(file_path)$runtime
if (!is.null(runtime) && grepl("^shiny", runtime)) {
  name <- basename(file_path)
  rmarkdown::run(name, dir = dirname(file_path), default_file = name, shiny_args = list(launch.browser = TRUE))
} else {
  output_file <- rmarkdown::render(file_path, output_dir = dirname(file_path), knit_root_dir = knit_root_directory)
  if (!is.na(result_file)) {
    cat(output_file, file = result_file)
  }
}
