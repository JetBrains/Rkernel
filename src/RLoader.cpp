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
#include "RStuff/RUtil.h"
#include "util/ContainerUtil.h"
#include "util/StringUtil.h"
#include <grpcpp/server_builder.h>
#include <limits>

const int MAX_PREVIEW_STRING_LENGTH = 200;
const int MAX_PREVIEW_PRINTED_COUNT = 20;

void getValueInfo(SEXP _var, ValueInfo* result) {
  ShieldSEXP var = _var;
  try {
    auto type = var.type();
    if (type == PROMSXP) {
      if (PRVALUE(var) == R_UnboundValue) {
        TextBuilder builder;
        builder.build(PRCODE(var));
        result->mutable_unevaluated()->set_code(builder.getText());
        return;
      }
      getValueInfo(PRVALUE(var), result);
      return;
    }
    ShieldSEXP classes = RI->classes(Rf_lang2(RI->quote, var));
    if (classes.type() == STRSXP) {
      for (int i = 0; i < classes.length(); ++i) {
        result->add_cls(stringEltUTF8(classes, i));
      }
    }
    if (type == LANGSXP || type == SYMSXP) {
      result->mutable_value()->set_iscomplete(true);
      result->mutable_value()->set_isvector(false);
      TextBuilder builder;
      builder.build(var);
      result->mutable_value()->set_textvalue(builder.getText());
    } else if (type == DOTSXP) {
      result->mutable_list()->set_length(Rf_xlength(var));
    } else if (type == CLOSXP || type == SPECIALSXP || type == BUILTINSXP) {
      result->mutable_function()->set_header(getFunctionHeader(var));
    } else if (type == ENVSXP) {
      result->mutable_environment()->set_name(asStringUTF8(RI->environmentName(var)));
    } else if (type == BCODESXP || type == WEAKREFSXP || type == EXTPTRSXP) {
      result->mutable_value()->set_iscomplete(true);
      result->mutable_value()->set_isvector(false);
      result->mutable_value()->set_textvalue(getPrintedValue(RI->unclass(Rf_lang2(RI->quote, var))));
    } else if (type == CHARSXP) {
      getValueInfo(Rf_ScalarString(var), result);
    } else if (Rf_inherits(var, "ggplot")) {
      result->mutable_graph();
    } else if (Rf_inherits(var, "data.frame")) {
      ValueInfo::DataFrame* dataFrame = result->mutable_dataframe();
      dataFrame->set_rows(asInt(RI->nrow(var)));
      dataFrame->set_cols(asInt(RI->ncol(var)));
    } else if (type == S4SXP) {
      result->mutable_value()->set_iss4(true);
    } else {
      if (Rf_isVector(var)) {
        ShieldSEXP dim = Rf_getAttrib(var, R_DimSymbol);
        if (dim.type() == INTSXP && dim.length() >= 2) {
          for (int i = 0; i < dim.length(); ++i) {
            result->mutable_matrix()->add_dim(asInt(dim[i]));
          }
          return;
        }
      }
      if (type == VECSXP || type == LISTSXP || type == EXPRSXP) {
        result->mutable_list()->set_length(var.length());
      } else {
        ValueInfo::Value *value = result->mutable_value();
        if (type == LGLSXP || type == INTSXP || type == REALSXP ||
            type == CPLXSXP || type == NILSXP) {
          R_xlen_t length = var.length();
          value->set_isvector(length > 1);
          ShieldSEXP unclassed =
              Rf_inherits(var, "factor") ? (SEXP)var : RI->unclass(var);
          if (length <= MAX_PREVIEW_PRINTED_COUNT) {
            value->set_textvalue(getPrintedValue(unclassed));
            value->set_iscomplete(true);
          } else {
            value->set_textvalue(getPrintedValue(RI->subscript(
                unclassed, RI->colon(1, MAX_PREVIEW_PRINTED_COUNT))));
            value->set_iscomplete(false);
          }
        } else if (type == STRSXP) {
          int length = var.length();
          value->set_isvector(length > 1);
          bool isComplete = length <= MAX_PREVIEW_PRINTED_COUNT;
          ShieldSEXP unclassed =
              Rf_inherits(var, "factor") ? (SEXP)var : RI->unclass(var);
          ShieldSEXP vector =
              isComplete
                  ? (SEXP)unclassed
                  : RI->subscript(unclassed,
                                  RI->colon(1, MAX_PREVIEW_PRINTED_COUNT));
          ShieldSEXP vectorPrefix =
              RI->substring(vector, 1, MAX_PREVIEW_STRING_LENGTH);
          ShieldSEXP nchar = RI->nchar(vectorPrefix);
          for (int i = 0; isComplete && i < vectorPrefix.length(); ++i) {
            if (!vectorPrefix.isNA(i) &&
                asInt(nchar[i]) == MAX_PREVIEW_STRING_LENGTH) {
              isComplete = false;
            }
          }
          value->set_textvalue(getPrintedValue(vectorPrefix));
          value->set_iscomplete(isComplete);
        } else {
          value->set_isvector(false);
          value->set_textvalue("");
          value->set_iscomplete(true);
        }
      }
    }
  } catch (RExceptionBase const& e) {
    result->mutable_error()->set_text(e.what());
  } catch (...) {
    result->mutable_error()->set_text("Error");
  }
}

Status RPIServiceImpl::loaderGetParentEnvs(ServerContext* context, const RRef* request, ParentEnvsResponse* response) {
  executeOnMainThread([&] {
    PrSEXP environment = dereference(*request);
    while (environment != R_EmptyEnv) {
      environment = environment.parentEnv();
      ParentEnvsResponse::EnvInfo* envInfo = response->add_envs();
      envInfo->set_name(asStringUTF8(RI->environmentName(environment)));
    }
  }, context, true);
  return Status::OK;
}

Status RPIServiceImpl::loaderGetVariables(ServerContext* context, const GetVariablesRequest* request, VariablesResponse* response) {
  executeOnMainThread([&] {
    ShieldSEXP obj = dereference(request->obj());
    R_xlen_t reqStart = request->start();
    R_xlen_t reqEnd = request->end();
    if (reqEnd == -1) reqEnd = std::numeric_limits<R_xlen_t>::max();
    if (obj.type() == ENVSXP) {
      response->set_isenv(true);
      ShieldSEXP ls = RI->ls(named("envir", obj), named("all.names", true));
      if (ls.type() != STRSXP) return;
      response->set_totalcount(ls.length());
      for (int i = std::max<R_xlen_t>(0, reqStart); i < std::min(ls.length(), reqEnd); ++i) {
        VariablesResponse::Variable* var = response->add_vars();
        var->set_name(stringEltUTF8(ls, i));
        getValueInfo(obj.getVar(stringEltNative(ls, i), false), var->mutable_value());
      }
      return;
    }
    response->set_isenv(false);
    if (obj.type() == LISTSXP || obj.type() == DOTSXP) {
      SEXP x = obj;
      response->set_totalcount(obj.length());
      int i = 0;
      while (x != R_NilValue && i < reqEnd) {
        if (i >= reqStart) {
          VariablesResponse::Variable* var = response->add_vars();
          if (TAG(x) != R_NilValue) {
            var->set_name(asStringUTF8(PRINTNAME(TAG(x))));
          }
          getValueInfo(CAR(x), var->mutable_value());
        }
        x = CDR(x);
        ++i;
      }
      return;
    }
    if (obj.type() != VECSXP && obj.type() != INTSXP && obj.type() != REALSXP && obj.type() != STRSXP &&
        obj.type() != RAWSXP && obj.type() != CPLXSXP && obj.type() != LGLSXP && obj.type() != EXPRSXP) {
      return;
    }
    R_xlen_t length = obj.length();
    response->set_totalcount(length);
    ShieldSEXP unclassed = Rf_inherits(obj, "factor") ? (SEXP)obj : RI->unclass(obj);
    ShieldSEXP filtered = RI->subscript(unclassed, RI->colon(reqStart + 1, std::min(length, reqEnd)));
    ShieldSEXP names = Rf_getAttrib(filtered, R_NamesSymbol);
    for (int i = 0; i < filtered.length(); ++i) {
      VariablesResponse::Variable* var = response->add_vars();
      var->set_name(stringEltUTF8(names, i));
      getValueInfo(RI->doubleSubscript(filtered, i + 1), var->mutable_value());
    }
  }, context, true);
  return Status::OK;
}

Status RPIServiceImpl::loaderGetLoadedNamespaces(ServerContext* context, const Empty*, StringList* response) {
  executeOnMainThread([&] {
    PrSEXP env = RI->globalEnv.parentEnv();
    while (env != R_EmptyEnv && env != R_NilValue) {
      std::string name = asStringUTF8(RI->environmentName(env));
      if (startsWith(name, "package:")) {
        response->add_list(name.substr(strlen("package:")));
      }
      env = env.parentEnv();
    }
    response->add_list("base");
  }, context, true);
  return Status::OK;
}

Status RPIServiceImpl::loaderGetValueInfo(ServerContext* context, const RRef* request, ValueInfo* response) {
  executeOnMainThread([&] {
    try {
      getValueInfo(dereference(*request), response);
    } catch (RExceptionBase const& e) {
      response->mutable_error()->set_text(e.what());
    } catch (...) {
      response->mutable_error()->set_text("Error");
    }
  }, context, true);
  return Status::OK;
}

Status RPIServiceImpl::getObjectSizes(ServerContext* context, const RRefList* request, Int64List* response) {
  executeOnMainThread([&] {
    for (RRef const& ref : request->refs()) {
      long long size;
      try {
        size = asInt64OrError(RI->objectSize(Rf_lang2(RI->quote, dereference(ref))));
      } catch (RExceptionBase const&) {
        size = -1;
      }
      response->add_list(size);
    }
  }, context, true);
  return Status::OK;
}
