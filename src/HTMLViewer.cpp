//  Rkernel is an execution kernel for R interpreter
//  Copyright (C) 2019 JetBrains s.r.o.
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "RPIServiceImpl.h"
#include "HTMLViewer.h"
#include "util/StringUtil.h"
#include "RStuff/RUtil.h"
#include "util/FileUtil.h"

void htmlViewerInit() {
  RI->evalCode(
    ".jetbrains_ther_old_browser <- getOption('browser')\n"
    "options(browser = function(url) {\n"
    "  if (grepl('^https?:', url)) {\n"
    "    if (!.Call('.jetbrains_processBrowseURL', url)) {\n"
    "      browseURL(url, .jetbrains_ther_old_browser)\n"
    "    }\n"
    "  } else {\n"
    "    .Call('.jetbrains_showFile', url, url)\n"
    "  }\n"
    "})\n"
    "options(pager = function(files, header = '', title = '', delete.file = TRUE) {\n"
    "  if (!is.logical(delete.file)) stop(\"'delete.file' must be logical\")\n"
    "  for (f in files) {\n"
    "    .Call('.jetbrains_showFile', normalizePath(f), title)\n"
    "    if (delete.file) unlink(f)\n"
    "  }\n"
    "})",
    R_GlobalEnv
  );
}

static std::string joinUrls(std::string u1, std::string const& u2) {
  if (startsWith(u2, "/")) return u2;
  while (!u1.empty() && u1.back() != '/') u1.pop_back();
  if (u1.empty()) u1 = "/";
  for (int i = 0; true; i += 3) {
    if (u2.substr(i, 3) == "../") {
      do u1.pop_back(); while (!u1.empty() && u1.back() != '/');
      if (u1.empty()) u1 = "/";
    } else {
      return u1 + u2.substr(i);
    }
  }
}

struct GetContentResult {
  bool success;
  std::string content;
  std::string url;
};

static GetContentResult getURLContent(std::string request, int maxRedirects = 5) {
  GetContentResult result = { false, "", request };
  if (startsWith(request, "http://127.0.0.1")) {
    int pos = strlen("https://127.0.0.1");
    while (pos < request.size() && request[pos] != '/') ++pos;
    request.erase(request.begin(), request.begin() + pos);
  }
  std::vector<std::string> args, argNames;
  for (int qPos = 0; qPos < request.size(); ++qPos) {
    if (request[qPos] == '?') {
      int start = qPos + 1;
      while (start < request.size()) {
        int eqPos = -1;
        int end = start;
        while (end < request.size() && request[end] != '&') {
          if (request[end] == '=') {
            eqPos = end;
          }
          ++end;
        }
        std::string name, value;
        if (eqPos == -1) {
          name = request.substr(start, end - start);
        } else {
          name = request.substr(start, eqPos - start);
          value = request.substr(eqPos + 1, end - (eqPos + 1));
        }
        args.push_back(value);
        argNames.push_back(name);
        start = end + 1;
      }
      request.erase(request.begin() + qPos, request.end());
      break;
    }
  }
  ShieldSEXP response = RI->httpd(request, makeCharacterVector(args, argNames));
  if (response.type() != VECSXP) return result;
  if (asInt(response["status code"]) == 302) {
    if (maxRedirects > 0) {
      std::string header = asStringUTF8(response["header"]);
      if (startsWith(header, "Location: ")) {
        return getURLContent(joinUrls(request, header.substr(strlen("Location: "))), maxRedirects - 1);
      }
    }
    return result;
  }

  PrSEXP payload = response["payload"];
  if (payload.type() == VECSXP) {
    payload = payload["payload"];
  }
  if (payload.type() == STRSXP) {
    result.content = asStringUTF8(RI->paste(payload, named("collapse", "\n")));
    result.success = true;
    return result;
  }

  ShieldSEXP file = response["file"];
  if (isScalarString(file)) {
    result.content = readWholeFile(asStringUTF8(file));
    result.success = true;
  }
  return result;
}

bool processBrowseURL(std::string const& url) {
  int port = asInt(RI->httpdPort());
  std::string prefix = "http://127.0.0.1:" + std::to_string(port) + "/";
  if (!startsWith(url, prefix)) return false;
  GetContentResult response = getURLContent(url);
  if (!response.success) return false;
  rpiService->showHelpHandler(response.content, response.url);
  return true;
}

Status RPIServiceImpl::httpdRequest(ServerContext* context, const StringValue* request, HttpdResponse* response) {
  executeOnMainThread([&] {
    GetContentResult result = getURLContent(request->value());
    response->set_success(result.success);
    response->set_content(std::move(result.content));
    response->set_url(std::move(result.url));
  }, context);
  return Status::OK;
}