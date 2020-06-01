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
#include <grpcpp/server_builder.h>
#include "RStuff/RUtil.h"
#include "debugger/SourceFileManager.h"
#include "EventLoop.h"
#include "RStuff/RObjects.h"

const int EVALUATE_AS_TEXT_MAX_LENGTH = 500000;

SEXP RPIServiceImpl::dereference(RRef const& ref) {
  switch (ref.ref_case()) {
    case RRef::kPersistentIndex: {
      int i = ref.persistentindex();
      return persistentRefStorage.has(i) ? (SEXP)persistentRefStorage[i] : R_NilValue;
    }
    case RRef::kGlobalEnv:
      return R_GlobalEnv;
    case RRef::kCurrentEnv:
      return currentEnvironment();
    case RRef::kSysFrameIndex: {
      int index = ref.sysframeindex();
      auto const& stack = rDebugger.getStack();
      if (index < 0 || index >= stack.size()) return R_NilValue;
      return stack[index].environment;
    }
    case RRef::kErrorStackSysFrameIndex: {
      int index = ref.errorstacksysframeindex();
      if (index < 0 || index >= lastErrorStack.size()) return R_NilValue;
      return lastErrorStack[index].environment;
    }
    case RRef::kMember: {
      ShieldSEXP env = dereference(ref.member().env());
      return env.getVar(ref.member().name());
    }
    case RRef::kParentEnv: {
      PrSEXP env = dereference(ref.parentenv().env());
      int count = ref.parentenv().index();
      for (int i = 0; i < count; ++i) {
        if (env.type() != ENVSXP || env == R_EmptyEnv) {
          return R_NilValue;
        }
        env = env.parentEnv();
      }
      return env;
    }
    case RRef::kExpression: {
      ShieldSEXP env = dereference(ref.expression().env());
      std::string code = ref.expression().code();
      return RI->eval(parseCode(code), named("envir", env));
    }
    case RRef::kListElement: {
      ShieldSEXP list = dereference(ref.listelement().list());
      long long index = ref.listelement().index();
      ShieldSEXP unclassed = Rf_inherits(list, "factor") ? (SEXP)list : RI->unclass(list);
      return RI->doubleSubscript(unclassed, index + 1);
    }
    case RRef::kAttributes: {
      ShieldSEXP x = dereference(ref.attributes());
      return RI->attributes(Rf_lang2(RI->quote, x));
    }
    default:
      return R_NilValue;
  }
}

Status RPIServiceImpl::copyToPersistentRef(ServerContext* context, const RRef* request, CopyToPersistentRefResponse* response) {
  executeOnMainThread([&] {
    try {
      response->set_persistentindex(persistentRefStorage.add(dereference(*request)));
    } catch (RExceptionBase const& e) {
      response->set_error(e.what());
    }
  }, context, true);
  return Status::OK;
}

Status RPIServiceImpl::disposePersistentRefs(ServerContext*, const PersistentRefList* request, Empty*) {
  std::vector<int> refs(request->indices().begin(), request->indices().end());
  eventLoopExecute([=] {
    for (int ref : refs) {
      if (persistentRefStorage.has(ref)) {
        persistentRefStorage.remove(ref);
      }
    }
  });
  return Status::OK;
}

Status RPIServiceImpl::evaluateAsText(ServerContext* context, const RRef* request, StringOrError* response) {
  executeOnMainThread([&] {
    try {
      PrSEXP value = dereference(*request);
      if (value.type() == STRSXP) {
        value = RI->substring(value, 1, EVALUATE_AS_TEXT_MAX_LENGTH);
      }
      response->set_value(getPrintedValueWithLimit(value, EVALUATE_AS_TEXT_MAX_LENGTH));
    } catch (RExceptionBase const& e) {
      response->set_error(e.what());
    } catch (...) {
      response->set_error("");
      throw;
    }
  }, context, true);
  return Status::OK;
}

Status RPIServiceImpl::evaluateAsBoolean(ServerContext* context, const RRef* request, BoolValue* response) {
  executeOnMainThread([&] {
    try {
      response->set_value(asBool(dereference(*request)));
    } catch (RExceptionBase const&) {
      response->set_value(false);
    }
  }, context, true);
  return Status::OK;
}


Status RPIServiceImpl::getDistinctStrings(ServerContext* context, const RRef* request, StringList* response) {
  executeOnMainThread([&] {
    ShieldSEXP object = dereference(*request);
    if (object.type() != STRSXP && !Rf_inherits(object, "factor")) {
      return;
    }
    ShieldSEXP vector = RI->asCharacter(RI->unique(object));
    int sumLength = 0;
    for (int i = 0; i < vector.length(); ++i) {
      if (!vector.isNA(i)) {
        std::string s = stringEltUTF8(vector, i);
        sumLength += s.size();
        if (sumLength > EVALUATE_AS_TEXT_MAX_LENGTH) break;
        response->add_list(s);
      }
    }
  }, context, true);
  return Status::OK;
}

Status RPIServiceImpl::loadObjectNames(ServerContext* context, const RRef* request, StringList* response) {
  executeOnMainThread([&] {
    ShieldSEXP names = RI->ls(dereference(*request), named("all.names", true));
    if (names.type() != STRSXP) return;
    for (int i = 0; i < names.length(); ++i) {
      response->add_list(stringEltUTF8(names, i));
    }
  }, context, true);
  return Status::OK;
}

Status RPIServiceImpl::findInheritorNamedArguments(ServerContext* context, const RRef* request, StringList* response) {
  executeOnMainThread([&] {
    ShieldSEXP jetbrainsEnv = RI->baseEnv.getVar(".jetbrains");
    ShieldSEXP func = jetbrainsEnv.getVar("findInheritorNamedArguments");
    ShieldSEXP result = func(dereference(*request));
    if (TYPEOF(result) != STRSXP) return;
    for (int i = 0; i < result.length(); ++i) {
      response->add_list(stringEltUTF8(result, i));
    }
  }, context, true);
  return Status::OK;
}

Status RPIServiceImpl::findDotsNamedArguments(ServerContext* context, const RRef* request, DotsNamedArguments* response) {
  executeOnMainThread([&] {
      ShieldSEXP jetbrainsEnv = RI->baseEnv.getVar(".jetbrains");
      ShieldSEXP func = jetbrainsEnv.getVar("findDotsNamedArgs");
      ShieldSEXP result = func(dereference(*request), named("depth", 2));
      if (TYPEOF(result) != VECSXP) return;
      for (int i = 0; i < result.length(); ++i) {
        ShieldSEXP elem = VECTOR_ELT(result, i);
        ShieldSEXP name = VECTOR_ELT(elem, 0);
        if (asBool(VECTOR_ELT(elem, 1))) {
          response->add_funargnames(stringEltUTF8(name, 0));
        }
        else {
          response->add_argnames(stringEltUTF8(name, 0));
        }
      }
  }, context, true);
  return Status::OK;
}

Status RPIServiceImpl::getTableColumnsInfo(ServerContext* context, const TableColumnsInfoRequest* request, TableColumnsInfo* response) {
  executeOnMainThread([&] {
    ShieldSEXP table = dereference(request->ref());
    if (!isDataFrame(table)) return;
    response->set_tabletype(
        Rf_inherits(table, "tbl_df") ? TableColumnsInfo_TableType_DPLYR :
        Rf_inherits(table, "data.table") ? TableColumnsInfo_TableType_DATA_TABLE :
        Rf_inherits(table, "data.frame") ? TableColumnsInfo_TableType_DATA_FRAME :
        TableColumnsInfo_TableType_UNKNOWN);

    ShieldSEXP names = RI->names(table);
    if (TYPEOF(names) != STRSXP) return;
    int ncol = asInt(RI->ncol(table));
    for (int i = 0; i < ncol; ++i) {
      TableColumnsInfo::Column *column = response->add_columns();
      column->set_name(stringEltUTF8(names, i));
      column->set_type(asStringUTF8(RI->paste(RI->classes(table[i]), named("collapse", ","))));
    }
  }, context, true);
  return Status::OK;
}

Status RPIServiceImpl::getFormalArguments(ServerContext* context, const RRef* request, StringList* response) {
  executeOnMainThread([&] {
    ShieldSEXP names = RI->names(RI->formals(dereference(*request)));
    if (TYPEOF(names) != STRSXP) return;
    for (int i = 0; i < names.length(); ++i) {
      response->add_list(stringEltUTF8(names, i));
    }
  }, context, true);
  return Status::OK;
}

Status RPIServiceImpl::getRMarkdownChunkOptions(ServerContext* context, const Empty*, StringList* response) {
  executeOnMainThread([&] {
    ShieldSEXP options = RI->evalCode("names(knitr::opts_chunk$get())", R_BaseEnv);
    if (TYPEOF(options) != STRSXP) return;
    for (int i = 0; i < options.length(); ++i) {
      response->add_list(stringEltUTF8(options, i));
    }
  }, context, true);
  return Status::OK;
}

Status RPIServiceImpl::getFunctionSourcePosition(ServerContext* context, const RRef* request, SourcePosition* response) {
  executeOnMainThread([&] {
    ShieldSEXP function = dereference(*request);
    std::string suggestedFileName;
    switch (request->ref_case()) {
      case RRef::kMember:
        suggestedFileName = request->member().name();
        break;
      case RRef::kExpression:
        suggestedFileName = request->expression().code();
        break;
      default:;
    }
    auto position = srcrefToPosition(sourceFileManager.getFunctionSrcref(function, suggestedFileName));
    response->set_fileid(position.first);
    response->set_line(position.second);
  }, context, true);
  return Status::OK;
}

Status RPIServiceImpl::getEqualityObject(ServerContext* context, const RRef* request, Int64Value* response) {
  executeOnMainThread([&] {
    try {
      response->set_value((long long)(SEXP)dereference(*request));
    } catch (RExceptionBase const&) {
      response->set_value(0);
    }
  }, context, true);
  return Status::OK;
}

void RPIServiceImpl::setValueImpl(RRef const& ref, SEXP value) {
  SHIELD(value);
  switch (ref.ref_case()) {
    case RRef::kMember: {
      ShieldSEXP env = dereference(ref.member().env());
      RI->assign(ref.member().name(), value, named("envir", env));
      break;
    }
    case RRef::kListElement: {
      ShieldSEXP list = dereference(ref.listelement().list());
      ShieldSEXP newList = RI->doubleSubscriptAssign(list, ref.listelement().index() + 1, value);
      setValueImpl(ref.listelement().list(), newList);
      break;
    }
    case RRef::kAttributes: {
      ShieldSEXP obj = dereference(ref.attributes());
      ShieldSEXP newObj = RI->attributesAssign(obj, value);
      setValueImpl(ref.attributes(), newObj);
      break;
    }
    default: {
      throw std::invalid_argument("Invalid reference for setValue");
    }
  }
}

Status RPIServiceImpl::setValue(ServerContext* context, const SetValueRequest* request, SetValueResponse* response) {
  executeOnMainThread([&] {
    try {
      setValueImpl(request->ref(), dereference(request->value()));
      response->mutable_ok();
    } catch (RExceptionBase const& e) {
      response->set_error(e.what());
    } catch (...) {
      response->set_error("Error");
      throw;
    }
  }, context, true);
  return Status::OK;
}
