#include "RStudioApi.h"

std::string currentExpr;

SEXP getSourceEditorContext() {
  RObject args;
  return toSEXP(rpiService->rStudioApiRequest(GET_SOURCE_EDITOR_CONTEXT_ID, args));
}

SEXP getConsoleEditorContext() {
  RObject args;
  return toSEXP(rpiService->rStudioApiRequest(GET_CONSOLE_EDITOR_CONTEXT_ID, args));
}

SEXP getActiveDocumentContext() {
  RObject args;
  return toSEXP(rpiService->rStudioApiRequest(GET_ACTIVE_DOCUMENT_CONTEXT_ID, args));
}

SEXP insertText(SEXP insertions, SEXP id) {
  RObject args;
  args.set_allocated_list(new RObject_List);
  args.mutable_list()->mutable_robjects()->Add(fromSEXP(insertions));
  args.mutable_list()->mutable_robjects()->Add(fromSEXP(id));
  return toSEXP(rpiService->rStudioApiRequest(INSERT_TEXT_ID, args));
}

SEXP sendToConsole(SEXP code, SEXP execute, SEXP echo, SEXP focus) {
  RObject args;
  args.set_allocated_list(new RObject_List);
  args.mutable_list()->mutable_robjects()->Add(fromSEXP(code));
  args.mutable_list()->mutable_robjects()->Add(fromSEXP(execute));
  args.mutable_list()->mutable_robjects()->Add(fromSEXP(echo));
  args.mutable_list()->mutable_robjects()->Add(fromSEXP(focus));
  args.mutable_list()->mutable_robjects()->Add(currentExpression());
  return toSEXP(rpiService->rStudioApiRequest(SEND_TO_CONSOLE_ID, args));
}

SEXP navigateToFile(SEXP filename, SEXP position) {
  RObject args;
  args.set_allocated_list(new RObject_List);
  args.mutable_list()->mutable_robjects()->Add(fromSEXP(filename));
  args.mutable_list()->mutable_robjects()->Add(fromSEXP(position));
  return toSEXP(rpiService->rStudioApiRequest(NAVIGATE_TO_FILE_ID, args));
}

SEXP getActiveProject() {
  RObject args;
  return toSEXP(rpiService->rStudioApiRequest(GET_ACTIVE_PROJECT_ID, args));
}

SEXP setSelectionRanges(SEXP ranges, SEXP id) {
  RObject args;
  args.set_allocated_list(new RObject_List);
  args.mutable_list()->mutable_robjects()->Add(fromSEXP(ranges));
  args.mutable_list()->mutable_robjects()->Add(fromSEXP(id));
  return toSEXP(rpiService->rStudioApiRequest(SET_SELECTION_RANGES_ID, args));
}

SEXP askForPassword(SEXP message) {
  return dialogHelper(message, ASK_FOR_PASSWORD_ID);
}

SEXP showQuestion(SEXP args) {
  return dialogHelper(args, SHOW_QUESTION_ID);
}

SEXP showPrompt(SEXP args) {
  return dialogHelper(args, SHOW_PROMPT_ID);
}

SEXP askForSecret(SEXP args) {
  return dialogHelper(args, ASK_FOR_SECRET_ID);
}

SEXP selectFile(SEXP args) {
  return dialogHelper(args, SELECT_FILE_ID);
}

SEXP selectDirectory(SEXP args) {
  return dialogHelper(args, SELECT_DIRECTORY_ID);
}

SEXP showDialog(SEXP args) {
  return dialogHelper(args, SHOW_DIALOG_ID);
}

SEXP updateDialog(SEXP args) {
  return dialogHelper(args, UPDATE_DIALOG_ID);
}

SEXP getThemeInfo() {
  RObject args;
  return toSEXP(rpiService->rStudioApiRequest(GET_THEME_INFO_ID, args));
}

SEXP jobRunScript(SEXP args) {
  return toSEXP(rpiService->rStudioApiRequest(JOB_RUN_SCRIPT_ID, fromSEXP(args)));
}

SEXP jobRemove(SEXP job) {
  return toSEXP(rpiService->rStudioApiRequest(JOB_REMOVE_ID, fromSEXP(job)));
}

SEXP toSEXP(const RObject &rObject) {
  switch (rObject.object_case()) {
    case RObject::kRString: {
      ShieldSEXP x = Rf_allocVector(STRSXP, rObject.rstring().strings_size());
      for (int i = 0; i < (int) rObject.rstring().strings_size(); ++i) {
        SET_STRING_ELT(x, i, mkCharUTF8(rObject.rstring().strings(i)));
      }
      return x;
    }
    case RObject::kRInt: {
      ShieldSEXP x = Rf_allocVector(INTSXP, rObject.rint().ints_size());
      for (int i = 0; i < (int) rObject.rint().ints_size(); ++i) {
        SET_INTEGER_ELT(x, i, rObject.rint().ints(i));
      }
      return x;
    }
    case RObject::kRDouble: {
      ShieldSEXP x = Rf_allocVector(REALSXP, rObject.rdouble().doubles_size());
      for (int i = 0; i < (int) rObject.rdouble().doubles_size(); ++i) {
        SET_REAL_ELT(x, i, rObject.rdouble().doubles(i));
      }
      return x;
    }
    case RObject::OBJECT_NOT_SET:
      return allocVector(VECSXP, 0);
    case RObject::kRnull:
      return R_NilValue;
    case RObject::kRboolean: {
      ShieldSEXP x = Rf_allocVector(LGLSXP, rObject.rboolean().booleans_size());
      for (int i = 0; i < (int) rObject.rboolean().booleans_size(); ++i) {
        SET_LOGICAL_ELT(x, i, rObject.rboolean().booleans(i));
      }
      return x;
    }
    case RObject::kList: {
      ShieldSEXP res = allocVector(VECSXP, rObject.list().robjects_size());
      for (size_t i = 0; i < rObject.list().robjects_size(); i++) {
        SET_VECTOR_ELT(res, i, toSEXP(rObject.list().robjects(i)));
      }
      return res;
    }
    case RObject::kNamedList: {
      ShieldSEXP names = Rf_allocVector(STRSXP, rObject.namedlist().robjects_size());
      for (int i = 0; i < (int) rObject.namedlist().robjects_size(); ++i) {
        SET_STRING_ELT(names, i, mkCharUTF8(rObject.namedlist().robjects(i).key()));
      }
      ShieldSEXP values = allocVector(VECSXP, rObject.namedlist().robjects_size());
      for (size_t i = 0; i < rObject.namedlist().robjects_size(); i++) {
        SET_VECTOR_ELT(values, i, toSEXP(rObject.namedlist().robjects(i).value()));
      }
      Rf_setAttrib(values, R_NamesSymbol, names);
      return values;
    }
    case RObject::kError: {
      error_return(rObject.error().data())
    }
  }
}

RObject fromSEXP(SEXP const &expr) {
  if (TYPEOF(expr) == VECSXP) {
    RObject result;
    result.set_allocated_list(new RObject_List);
    for (size_t i = 0; i < LENGTH(expr); i++) {
      result.mutable_list()->mutable_robjects()->Add(fromSEXP(VECTOR_ELT(expr, i)));
    }
    return result;
  } else if (TYPEOF(expr) == REALSXP) {
    RObject result;
    result.set_allocated_rdouble(new RObject_RDouble);
    for (size_t i = 0; i < LENGTH(expr); i++) {
      result.mutable_rdouble()->add_doubles(REAL_ELT(expr, i));
    }
    return result;
  } else if (TYPEOF(expr) == INTSXP) {
    RObject result;
    result.set_allocated_rint(new RObject_RInt);
    for (size_t i = 0; i < LENGTH(expr); i++) {
      result.mutable_rint()->add_ints(INTEGER_ELT(expr, i));
    }
    return result;
  } else if (TYPEOF(expr) == STRSXP) {
    RObject result;
    result.set_allocated_rstring(new RObject_RString);
    for (size_t i = 0; i < LENGTH(expr); i++) {
      result.mutable_rstring()->add_strings(asStringUTF8OrError(STRING_ELT(expr, i)));
    }
    return result;
  } else if (TYPEOF(expr) == NILSXP) {
    RObject result;
    result.set_allocated_rnull(new RObject_RNull);
    return result;
  } else if (TYPEOF(expr) == LGLSXP) {
    RObject result;
    result.set_allocated_rboolean(new RObject_RBoolean);
    for (size_t i = 0; i < LENGTH(expr); i++) {
      result.mutable_rboolean()->add_booleans(LOGICAL_ELT(expr, i));
    }
    return result;
  } else throw std::exception();
}

SEXP dialogHelper(SEXP args, int32_t id) {
  RObject args_;
  args_.set_allocated_list(new RObject_List);
  args_.mutable_list()->mutable_robjects()->Add(fromSEXP(args));
  return toSEXP(rpiService->rStudioApiRequest(id, args_));
}

RObject currentExpression() {
  RObject result;
  result.set_allocated_rstring(new RObject_RString);
  result.mutable_rstring()->add_strings(currentExpr);
  return result;
}