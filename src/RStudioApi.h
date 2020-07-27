#ifndef RWRAPPER_RSTUDIOAPI_H
#define RWRAPPER_RSTUDIOAPI_H

#include "RPIServiceImpl.h"
#include "RStuff/MySEXP.h"

#define GET_SOURCE_EDITOR_CONTEXT_ID 0
#define INSERT_TEXT_ID 1
#define SEND_TO_CONSOLE_ID 2
#define GET_CONSOLE_EDITOR_CONTEXT_ID 3
#define NAVIGATE_TO_FILE_ID 4
#define GET_ACTIVE_PROJECT_ID 5
#define GET_ACTIVE_DOCUMENT_CONTEXT_ID 6

SEXP getSourceEditorContext();

SEXP getConsoleEditorContext();

SEXP getActiveDocumentContext();

SEXP insertText(SEXP insertions, SEXP id);

SEXP sendToConsole(SEXP code, SEXP execute, SEXP echo, SEXP focus);

SEXP navigateToFile(SEXP filename, SEXP position);

SEXP getActiveProject();

SEXP toSEXP(const RObject &rObject);

RObject fromSEXP(const SEXP &expr);

#endif //RWRAPPER_RSTUDIOAPI_H
