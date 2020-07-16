#ifndef RWRAPPER_RSTUDIOAPI_H
#define RWRAPPER_RSTUDIOAPI_H

#include <cstdint>
#include <vector>
#include "RPIServiceImpl.h"
#include <grpcpp/server_builder.h>

#define GET_SOURCE_EDITOR_CONTEXT_ID 0

SEXP getSourceEditorContext();

SEXP toSEXP(const RObject &rObject);

#endif //RWRAPPER_RSTUDIOAPI_H
