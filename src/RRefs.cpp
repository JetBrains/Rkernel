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
#include <Rcpp.h>
#include <grpcpp/server_builder.h>
#include "RObjects.h"
#include "util/RUtil.h"
#include "debugger/SourceFileManager.h"
#include "EventLoop.h"

const int EVALUATE_AS_TEXT_MAX_LENGTH = 500000;

Rcpp::RObject RPIServiceImpl::dereference(RRef const& ref) {
  switch (ref.ref_case()) {
    case RRef::kPersistentIndex: {
      int i = ref.persistentindex();
      return persistentRefStorage.has(i) ? persistentRefStorage[i] : RI->nil;
    }
    case RRef::kGlobalEnv:
      return Rcpp::as<Rcpp::RObject>(RI->globalEnv);
    case RRef::kCurrentEnv:
      return Rcpp::as<Rcpp::RObject>(currentEnvironment());
    case RRef::kSysFrameIndex: {
      int index = ref.sysframeindex();
      return rDebugger.getFrame(index);
    }
    case RRef::kErrorStackSysFrameIndex: {
      int index = ref.errorstacksysframeindex();
      return rDebugger.getErrorStackFrame(index);
    }
    case RRef::kMember:
      return Rcpp::as<Rcpp::Environment>(dereference(ref.member().env())).get(mkStringUTF8(ref.member().name()));
    case RRef::kParentEnv: {
      Rcpp::Environment env = Rcpp::as<Rcpp::Environment>(dereference(ref.parentenv().env()));
      int count = ref.parentenv().index();
      for (int i = 0; i < count; ++i) {
        if (env == Rcpp::Environment::empty_env()) {
          return RI->nil;
        }
        env = env.parent();
      }
      return Rcpp::as<Rcpp::RObject>(env);
    }
    case RRef::kExpression: {
      Rcpp::Environment env = Rcpp::as<Rcpp::Environment>(dereference(ref.expression().env()));
      std::string code = ref.expression().code();
      return RI->eval(parseCode(code), Rcpp::Named("envir", env));
    }
    case RRef::kListElement: {
      Rcpp::List list = Rcpp::as<Rcpp::List>(dereference(ref.listelement().list()));
      int index = ref.listelement().index();
      return list[index];
    }
    default:
      return RI->nil;
  }
}

Status RPIServiceImpl::copyToPersistentRef(ServerContext* context, const RRef* request, CopyToPersistentRefResponse* response) {
  executeOnMainThread([&] {
    try {
      response->set_persistentindex(persistentRefStorage.add(dereference(*request)));
    } catch (Rcpp::eval_error const& e) {
      response->set_error(e.what());
    }
  }, context);
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

Status RPIServiceImpl::evaluateAsText(ServerContext* context, const RRef* request, StringValue* response) {
  executeOnMainThread([&] {
    try {
      Rcpp::RObject value = dereference(*request);
      if (Rcpp::as<std::string>(RI->type(value)) == "character") {
        value = RI->substring(value, 1, EVALUATE_AS_TEXT_MAX_LENGTH);
      }
      response->set_value(getPrintedValueWithLimit(value, EVALUATE_AS_TEXT_MAX_LENGTH));
    } catch (Rcpp::eval_error const& e) {
      response->set_value(e.what());
    }
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::evaluateAsBoolean(ServerContext* context, const RRef* request, BoolValue* response) {
  executeOnMainThread([&] {
    try {
      response->set_value(Rcpp::as<bool>(dereference(*request)));
    } catch (Rcpp::eval_error const&) {
      response->set_value(false);
    }
  }, context);
  return Status::OK;
}


Status RPIServiceImpl::getDistinctStrings(ServerContext* context, const RRef* request, StringList* response) {
  executeOnMainThread([&] {
    Rcpp::RObject object = dereference(*request);
    if (!Rcpp::is<Rcpp::CharacterVector>(object) && !Rf_inherits(object, "factor")) {
      return;
    }
    Rcpp::CharacterVector vector = RI->unique(object);
    int sumLength = 0;
    for (int i = 0; i < vector.length(); ++i) {
      if (!vector.is_na(vector[i])) {
        std::string s = translateToUTF8(vector[i]);
        sumLength += s.length();
        if (sumLength > EVALUATE_AS_TEXT_MAX_LENGTH) break;
        response->add_list(s);
      }
    }
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::loadObjectNames(ServerContext* context, const RRef* request, StringList* response) {
  executeOnMainThread([&] {
    Rcpp::CharacterVector names = RI->ls(dereference(*request), Rcpp::Named("all.names", true));
    for (const char* x : names) {
      response->add_list(x);
    }
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::findAllNamedArguments(ServerContext* context, const RRef* request, StringList* response) {
  executeOnMainThread([&] {
    Rcpp::Environment jetbrainsEnv = RI->globalEnv[".jetbrains"];
    Rcpp::Function func = jetbrainsEnv["findAllNamedArguments"];
    Rcpp::RObject result = func(dereference(*request));
    if (!Rcpp::is<Rcpp::CharacterVector>(result)) {
      return;
    }
    for (const char* x : Rcpp::as<Rcpp::CharacterVector>(result)) {
      response->add_list(x);
    }
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::getTableColumnsInfo(ServerContext* context, const TableColumnsInfoRequest* request, TableColumnsInfo* response) {
  executeOnMainThread([&] {
    Rcpp::RObject tableObj = dereference(request->ref());
    if (!Rcpp::is<Rcpp::DataFrame>(tableObj)) return;

    response->set_tabletype(Rcpp::as<bool>(RI->inherits(tableObj, "tbl_df")) ? TableColumnsInfo_TableType_DPLYR :
     (Rcpp::as<bool>(RI->inherits(tableObj, "data.table")) ? TableColumnsInfo_TableType_DATA_TABLE :
     (Rcpp::as<bool>(RI->inherits(tableObj, "data.frame")) ? TableColumnsInfo_TableType_DATA_FRAME
                                                           : TableColumnsInfo_TableType_UNKNOWN)));

    Rcpp::DataFrame table = tableObj;
    Rcpp::CharacterVector names = table.names();
    for (int i = 0; i < (int)table.ncol(); ++i) {
      TableColumnsInfo::Column *column = response->add_columns();
      column->set_name(translateToUTF8(names[i]));
      column->set_type(translateToUTF8(RI->paste(RI->classes(table[i]), Rcpp::Named("collapse", ","))));
    }
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::getFormalArguments(ServerContext* context, const RRef* request, StringList* response) {
  executeOnMainThread([&] {
    Rcpp::RObject names = RI->names(RI->formals(dereference(*request)));
    if (Rcpp::is<Rcpp::CharacterVector>(names)) {
      for (const char* s : Rcpp::as<Rcpp::CharacterVector>(names)) {
        response->add_list(s);
      }
    }
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::getRMarkdownChunkOptions(ServerContext* context, const Empty*, StringList* response) {
    executeOnMainThread([&] {
        Rcpp::CharacterVector options = RI->evalCode("names(knitr::opts_chunk$get())", RI->baseEnv);
        for (const char* s : options) {
            response->add_list(s);
        }
    }, context);
    return Status::OK;
}

Status RPIServiceImpl::getFunctionSourcePosition(ServerContext* context, const RRef* request, SourcePosition* response) {
  executeOnMainThread([&] {
    Rcpp::RObject function = dereference(*request);
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
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::getEqualityObject(ServerContext* context, const RRef* request, Int64Value* response) {
  executeOnMainThread([&] {
    try {
      response->set_value((long long)(SEXP)dereference(*request));
    } catch (Rcpp::eval_error const&) {
      response->set_value(0);
    }
  }, context);
  return Status::OK;
}

void RPIServiceImpl::setValueImpl(RRef const& ref, Rcpp::RObject const& value) {
  switch (ref.ref_case()) {
    case RRef::kMember: {
      Rcpp::RObject env = dereference(ref.member().env());
      RI->assign(mkStringUTF8(ref.member().name()), value, Rcpp::Named("envir", env));
      break;
    }
    case RRef::kListElement: {
      Rcpp::RObject list = Rcpp::as<Rcpp::RObject>(dereference(ref.listelement().list()));
      Rcpp::RObject newList = RI->doubleSubscriptAssign(list, ref.listelement().index() + 1, value);
      setValueImpl(ref.listelement().list(), newList);
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
    } catch (Rcpp::eval_error const& e) {
      response->set_error(e.what());
    } catch (...) {
      response->set_error("Error");
    }
  }, context);
  return Status::OK;
}
