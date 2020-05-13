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
#include "Session.h"
#include "Options.h"
#include "RStuff/RObjects.h"
#include "debugger/SourceFileManager.h"
#include <Rinternals.h>
#include <signal.h>
#include <R_ext/RStartup.h>
#include "CrashReport.h"

static const char* SAVED_DATA_ENV = ".jetbrains_saved_data_env";

SessionManager sessionManager;

static void loadWorkspace(std::string const& path);
static void initSegvHandler();

void SessionManager::init() {
  initSegvHandler();
}

void SessionManager::quit() {
  if (saveOnExit) saveWorkspace();
}

void SessionManager::saveWorkspace(std::string const& path) {
  if (path == "") {
    if (workspaceFile != "") {
      saveWorkspace(workspaceFile);
    }
    return;
  }
  WithDebuggerEnabled with(false);
  ShieldSEXP oldWD = RI->getwd();
  try {
    ShieldSEXP savedDataEnv = RI->globalEnv.assign(SAVED_DATA_ENV, RI->newEnv());
    savedDataEnv.assign("SourceFileManager", sourceFileManager.saveState());
    std::string directory = asStringUTF8(RI->dirName(path));
    if (!asBool(RI->dirExists(directory))) {
      RI->dirCreate(directory);
    }
    RI->setwd(directory);
    RI->saveImage(RI->baseName(path));
    RI->cat("Workspace saved to", path, "\n", named("file", RI->stdErr()));
  } catch (RError const& e) {
    RI->cat("Failed to save workspace:", e.what(), "\n", named("file", RI->stdErr()));
  }
  try {
    RI->rm(SAVED_DATA_ENV, named("envir", R_GlobalEnv));
    RI->setwd(oldWD);
  } catch (RError const&) {
  }
}

void SessionManager::loadWorkspace(std::string const& path) {
  if (path == "") {
    if (workspaceFile != "") {
      loadWorkspace(workspaceFile);
    }
    return;
  }
  ShieldSEXP oldWD = RI->getwd();
  try {
    if (!asBool(RI->fileExists(path))) return;
    std::string directory = asStringUTF8(RI->dirName(path));
    RI->setwd(directory);
    RI->sysLoadImage(RI->baseName(path), true);
    ShieldSEXP savedDataEnv = RI->globalEnv[SAVED_DATA_ENV];
    if (savedDataEnv.type() == ENVSXP) {
      sourceFileManager.loadState(savedDataEnv["SourceFileManager"]);
      RI->rm(SAVED_DATA_ENV, named("envir", R_GlobalEnv));
    }
    RI->cat("Workspace restored from", path, "\n", named("file", RI->stdErr()));
  } catch (RError const& e) {
    RI->cat("Failed to restore workspace:", e.what(), "\n", named("file", RI->stdErr()));
  }
  try {
    RI->setwd(oldWD);
  } catch (RError const&) {
  }
}

extern "C" {
void NORET Rf_jump_to_toplevel();
LibExtern uintptr_t R_CStackLimit;
LibExtern uintptr_t R_CStackStart;
LibExtern int R_CStackDir;
}

#if defined(_WIN32_WINNT)
static void sigsegvHandler(int signum) {
  const char* s;
  /* need to take off stack checking as stack base has changed */
  R_CStackLimit = (uintptr_t) -1;
  /* Do not translate these messages */
  REprintf("\n *** caught %s ***\n", signum == SIGILL ? "illegal operation" : "segfault");

  if (!commandLineOptions.crashReportFile.empty()) {
    saveRWrapperCrashReport(commandLineOptions.crashReportFile);
  }

  sessionManager.quit();
  R_CleanTempDir();
  signal(signum, SIG_DFL);
  raise(signum);
}

static void initSegvHandler() {
  signal(SIGSEGV, sigsegvHandler);
  signal(SIGILL, sigsegvHandler);
}
#else
// This is mostly copy-paste from R source code
static void sigsegvHandler(int signum, siginfo_t* ip, void*) {
  const char* s;

  /* First check for stack overflow if we know the stack position.
     We assume anything within 16Mb beyond the stack end is a stack overflow.
   */
  if (signum == SIGSEGV && ip != nullptr && ip != (siginfo_t*) 1 && (intptr_t) R_CStackStart != -1) {
    uintptr_t addr = (uintptr_t) ip->si_addr;
    intptr_t diff = (R_CStackDir > 0) ? R_CStackStart - addr : addr - R_CStackStart;
    uintptr_t upper = 0x1000000;  /* 16Mb */
    if ((intptr_t) R_CStackLimit != -1) upper += R_CStackLimit;
    if (diff > 0 && diff < upper) {
      REprintf("Error: segfault from C stack overflow\n");
#if defined(linux) || defined(__linux__) || defined(__sun) || defined(sun)
      sigset_t ss;
      sigaddset(&ss, signum);
      sigprocmask(SIG_UNBLOCK, &ss, NULL);
#endif
      Rf_jump_to_toplevel();
    }
  }

  /* need to take off stack checking as stack base has changed */
  R_CStackLimit = (uintptr_t) -1;

  /* Do not translate these messages */
  REprintf("\n *** caught %s ***\n", signum == SIGILL ? "illegal operation" : signum == SIGBUS ? "bus error" : "segfault");
  if (ip != nullptr) {
    if (signum == SIGILL) {
      switch (ip->si_code) {
#ifdef ILL_ILLOPC
        case ILL_ILLOPC:
          s = "illegal opcode";
          break;
#endif
#ifdef ILL_ILLOPN
        case ILL_ILLOPN:
          s = "illegal operand";
          break;
#endif
#ifdef ILL_ILLADR
        case ILL_ILLADR:
          s = "illegal addressing mode";
          break;
#endif
#ifdef ILL_ILLTRP
        case ILL_ILLTRP:
          s = "illegal trap";
          break;
#endif
#ifdef ILL_COPROC
        case ILL_COPROC:
          s = "coprocessor error";
          break;
#endif
        default:
          s = "unknown";
          break;
      }
    } else if (signum == SIGBUS)
      switch (ip->si_code) {
#ifdef BUS_ADRALN
        case BUS_ADRALN:
          s = "invalid alignment";
          break;
#endif
#ifdef BUS_ADRERR /* not on macOS, apparently */
        case BUS_ADRERR:
          s = "non-existent physical address";
          break;
#endif
#ifdef BUS_OBJERR /* not on macOS, apparently */
        case BUS_OBJERR:
          s = "object specific hardware error";
          break;
#endif
        default:
          s = "unknown";
          break;
      }
    else
      switch (ip->si_code) {
#ifdef SEGV_MAPERR
        case SEGV_MAPERR:
          s = "memory not mapped";
          break;
#endif
#ifdef SEGV_ACCERR
        case SEGV_ACCERR:
          s = "invalid permissions";
          break;
#endif
        default:
          s = "unknown";
          break;
      }
    REprintf("address %p, cause '%s'\n", ip->si_addr, s);
  }

  if (!commandLineOptions.crashReportFile.empty()) {
    saveRWrapperCrashReport(commandLineOptions.crashReportFile);
  }

  sessionManager.quit();
  R_CleanTempDir();
  signal(signum, SIG_DFL);
  raise(signum);
}

static void initSegvHandler() {
  struct sigaction sa;
  sa.sa_sigaction = sigsegvHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;
  sigaction(SIGSEGV, &sa, nullptr);
  sigaction(SIGILL, &sa, nullptr);
#ifdef SIGBUS
  sigaction(SIGBUS, &sa, nullptr);
#endif
}
#endif

Status RPIServiceImpl::setSaveOnExit(ServerContext*, const BoolValue* request, Empty*) {
  sessionManager.saveOnExit = request->value();
  return Status::OK;
}
