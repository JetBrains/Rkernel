#include "RStudioApi.h"
#include "RStuff/RObjects.h"

SEXP getSourceEditorContext() {
  RObject args;
  return toSEXP(rpiService->rStudioApiRequest(GET_SOURCE_EDITOR_CONTEXT_ID, args));
}

SEXP getConsoleEditorContext() {
  RObject args;
  return toSEXP(rpiService->rStudioApiRequest(GET_CONSOLE_EDITOR_CONTEXT_ID, args));
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
  return toSEXP(rpiService->rStudioApiRequest(SEND_TO_CONSOLE_ID, args));
}

SEXP toSEXP(const RObject &rObject) {
  switch (rObject.object_case()) {
    case RObject::kRString:
      return mkStringUTF8(rObject.rstring().string());
    case RObject::kRInt:
      return ScalarInteger(rObject.rint().int_());
    case RObject::kRDouble:
      return ScalarReal(rObject.rdouble().double_());
    case RObject::OBJECT_NOT_SET:
      return allocVector(VECSXP, 0);
    case RObject::kRnull:
      return R_NilValue;
    case RObject::kRboolean:
      return ScalarLogical(rObject.rboolean().boolean());
    case RObject::kList:
      ShieldSEXP res = allocVector(VECSXP, rObject.list().robjects_size());
      for (size_t i = 0; i < rObject.list().robjects_size(); i++) {
        SET_VECTOR_ELT(res, i, toSEXP(rObject.list().robjects(i)));
      }
      return res;
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
    if (Rf_xlength(expr) == 1) {
      result.set_allocated_rdouble(new RObject_RDouble);
      result.mutable_rdouble()->set_double_((asDoubleOrError(expr)));
    } else {
      result.set_allocated_list(new RObject_List);
      for (size_t i = 0; i < LENGTH(expr); i++) {
        auto v = new RObject;
        v->set_allocated_rdouble(new RObject_RDouble);
        v->mutable_rdouble()->set_double_(REAL_ELT(expr, i));
        result.mutable_list()->mutable_robjects()->AddAllocated(v);
      }
    }
    return result;
  } else if (TYPEOF(expr) == INTSXP) {
    RObject result;
    if (Rf_xlength(expr) == 1) {
      result.set_allocated_rint(new RObject_RInt);
      result.mutable_rint()->set_int_((asInt64OrError(expr)));
    } else {
      result.set_allocated_list(new RObject_List);
      for (size_t i = 0; i < LENGTH(expr); i++) {
        auto v = new RObject;
        v->set_allocated_rint(new RObject_RInt);
        v->mutable_rint()->set_int_(INTEGER_ELT(expr, i));
        result.mutable_list()->mutable_robjects()->AddAllocated(v);
      }
    }
    return result;
  } else if (TYPEOF(expr) == STRSXP) {
    RObject result;
    if (Rf_xlength(expr) == 1) {
      result.set_allocated_rstring(new RObject_RString);
      result.mutable_rstring()->set_string((asStringUTF8OrError(expr)));
    } else {
      result.set_allocated_list(new RObject_List);
      for (size_t i = 0; i < LENGTH(expr); i++) {
        auto v = new RObject;
        v->set_allocated_rstring(new RObject_RString);
        v->mutable_rstring()->set_string(asStringUTF8OrError(STRING_ELT(expr, i)));
        result.mutable_list()->mutable_robjects()->AddAllocated(v);
      }
    }
    return result;
  } else if (TYPEOF(expr) == NILSXP) {
    RObject result;
    result.set_allocated_rnull(new RObject_RNull);
    return result;
  } else if (TYPEOF(expr) == LGLSXP) {
    RObject result;
    if (Rf_xlength(expr) == 1) {
      result.set_allocated_rboolean(new RObject_RBoolean);
      result.mutable_rboolean()->set_boolean((asBoolOrError(expr)));
    } else {
      result.set_allocated_list(new RObject_List);
      for (size_t i = 0; i < LENGTH(expr); i++) {
        auto v = new RObject;
        v->set_allocated_rboolean(new RObject_RBoolean);
        v->mutable_rboolean()->set_boolean(LOGICAL_ELT(expr, i));
        result.mutable_list()->mutable_robjects()->AddAllocated(v);
      }
    }
    return result;
  } else throw std::exception();
}
