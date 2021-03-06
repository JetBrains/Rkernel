#include "RStudioApi.h"

std::string currentExpr;

SEXP rStudioApiHelper(SEXP args, int id) {
  return toSEXP(rpiService->rStudioApiRequest(id, fromSEXP(args)));
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
        INTEGER(x)[i] = rObject.rint().ints(i);
      }
      return x;
    }
    case RObject::kRDouble: {
      ShieldSEXP x = Rf_allocVector(REALSXP, rObject.rdouble().doubles_size());
      for (int i = 0; i < (int) rObject.rdouble().doubles_size(); ++i) {
        REAL(x)[i] = rObject.rdouble().doubles(i);
      }
      return x;
    }
    case RObject::OBJECT_NOT_SET:
      return allocVector(VECSXP, 0);
    case RObject::kRNull:
      return R_NilValue;
    case RObject::kRBoolean: {
      ShieldSEXP x = Rf_allocVector(LGLSXP, rObject.rboolean().booleans_size());
      for (int i = 0; i < (int) rObject.rboolean().booleans_size(); ++i) {
        LOGICAL(x)[i] = rObject.rboolean().booleans(i);
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
      result.mutable_rdouble()->add_doubles(REAL(expr)[i]);
    }
    return result;
  } else if (TYPEOF(expr) == INTSXP) {
    RObject result;
    result.set_allocated_rint(new RObject_RInt);
    for (size_t i = 0; i < LENGTH(expr); i++) {
      result.mutable_rint()->add_ints(INTEGER(expr)[i]);
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
      result.mutable_rboolean()->add_booleans(LOGICAL(expr)[i]);
    }
    return result;
  } else throw std::exception();
}

RObject currentExpression() {
  RObject result;
  result.set_allocated_rstring(new RObject_RString);
  result.mutable_rstring()->add_strings(currentExpr);
  return result;
}
