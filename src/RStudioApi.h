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
#define SET_SELECTION_RANGES_ID 7
#define ASK_FOR_PASSWORD_ID 8
#define SHOW_QUESTION_ID 9
#define SHOW_PROMPT_ID 10
#define ASK_FOR_SECRET_ID 11
#define SELECT_FILE_ID 12
#define SELECT_DIRECTORY_ID 13
#define SHOW_DIALOG_ID 14
#define UPDATE_DIALOG_ID 15
#define GET_THEME_INFO_ID 16
#define JOB_RUN_SCRIPT_ID 17
#define JOB_REMOVE_ID 18
#define JOB_SET_STATE_ID 19
#define RESTART_SESSION_ID 20

extern std::string currentExpr;

SEXP getSourceEditorContext();

SEXP getConsoleEditorContext();

SEXP getActiveDocumentContext();

SEXP insertText(SEXP insertions, SEXP id);

SEXP sendToConsole(SEXP code, SEXP execute, SEXP echo, SEXP focus);

SEXP navigateToFile(SEXP filename, SEXP position);

SEXP getActiveProject();

SEXP setSelectionRanges(SEXP ranges, SEXP id);

SEXP askForPassword(SEXP message);

SEXP showQuestion(SEXP args);

SEXP showPrompt(SEXP args);

SEXP askForSecret(SEXP args);

SEXP selectFile(SEXP args);

SEXP selectDirectory(SEXP args);

SEXP showDialog(SEXP args);

SEXP updateDialog(SEXP args);

SEXP getThemeInfo();

SEXP jobRunScript(SEXP args);

SEXP jobRemove(SEXP job);

SEXP restartSession(SEXP command);

SEXP toSEXP(const RObject &rObject);

RObject fromSEXP(const SEXP &expr);

SEXP dialogHelper(SEXP args, int32_t id);

RObject currentExpression();

#endif //RWRAPPER_RSTUDIOAPI_H
