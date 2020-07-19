#ifndef RWRAPPER_RSTUDIOAPI_H
#define RWRAPPER_RSTUDIOAPI_H

#include <cstdint>
#include <vector>
#include "RPIServiceImpl.h"
#include <grpcpp/server_builder.h>

#define GET_SOURCE_EDITOR_CONTEXT_ID 0
#define INSERT_TEXT_ID 1
#define SEND_TO_CONSOLE_ID 2
#define GET_CONSOLE_EDITOR_CONTEXT_ID 3

SEXP getSourceEditorContext();

SEXP getConsoleEditorContext();

SEXP insertText(SEXP insertions, SEXP id);

SEXP sendToConsole(SEXP code, SEXP execute, SEXP echo, SEXP focus);

SEXP toSEXP(const RObject &rObject);

RObject fromSEXP(const SEXP &expr);

#endif //RWRAPPER_RSTUDIOAPI_H
