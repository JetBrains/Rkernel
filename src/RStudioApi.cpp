#include "RStudioApi.h"

SEXP getSourceEditorContext() {
  RObject args;
  return toSEXP(rpiService->rStudioApiRequest(GET_SOURCE_EDITOR_CONTEXT_ID, args));
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
    case RObject::kList:
      ShieldSEXP res = allocVector(VECSXP, rObject.list().robjects_size());
      for (size_t i = 0; i < rObject.list().robjects_size(); i++) {
        SET_VECTOR_ELT(res, i, toSEXP(rObject.list().robjects(i)));
      }
      return res;
  }
}