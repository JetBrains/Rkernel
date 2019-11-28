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

  download_pandoc <- function(library_path, archive_extension, URL) {
    archive_path <- paste0(library_path, "/pandoc.", archive_extension)
    download.file(URL, destfile = archive_path, mode="wb")
    archive_function <- if (archive_extension == "zip") unzip else untar
    archive_function(archive_path, exdir = library_path)
    file.remove(archive_path)
  }

  tune_pandoc <- function(library_path, executable_name) {
    pandoc_folders <- list.files(library_path, pattern="pandoc*")
    pandoc_folder <- head(pandoc_folders, 1)
    pandoc_folder_absolute <- paste0(library_path, "/", pandoc_folder)
    new_path <- paste0(library_path, "/pandoc")
    file.rename(pandoc_folder_absolute, new_path)

    # Some strange things are going on here.
    # You need to note that MacOS and Linux versions of archive
    # contains additional directory "bin" but Windows one doesn't.
    # In order to unify process it's convenient to move contents of "pandoc/bin" to "pandoc"
    bin_path <- paste0(new_path, "/bin")
    lapply(list.files(bin_path), function(name) {
      full_name <- paste0(bin_path, "/", name)
      new_name <- paste0(new_path, "/", name)
      file.rename(full_name, new_name)
    })

    pandoc_executable_path <- paste0(new_path, "/", executable_name)
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
  download_pandoc(library_path, archive_extension, URL)
  tune_pandoc(library_path, executable_name)
}

args = commandArgs(TRUE)

if (length(args) != 3) {
    warning("Usage: render_markdown.R <library_path> <file_path> <render_directory>")
    quit(save = "no", status = 1, runLast = FALSE)
}

library_path <- args[1]
file_path <- args[2]
knit_root_directory <- args[3]

# RMarkdown looks for Pandoc in PATH and RSTUDIO_PANDOC envs
# We don't want to touch RStudio's envs that's why we'd rather adjust current PATH
old_path <- Sys.getenv("PATH")
pandoc_path <- paste0(library_path, "/pandoc")
path_separator <- if (Sys.info()["sysname"] == "Windows") ";" else ":"
new_path <- paste0(old_path, path_separator, pandoc_path)
Sys.setenv(PATH = new_path)

# First of all, we need to ensure Pandoc is really here (very important)
minimum_pandoc_version <- "1.12.3"  # Note: it's hard coded into code for `rmarkdown::render` (No kidding!)
if (!rmarkdown::pandoc_available(minimum_pandoc_version)) {
  install_pandoc(library_path)
}

rmarkdown::render(file_path, output_dir = dirname(file_path), knit_root_dir = knit_root_directory)
