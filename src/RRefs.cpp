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
      return 0 <= index && index < sysFrames.size() ? Rcpp::as<Rcpp::RObject>(sysFrames[index]) : RI->nil;
    }
    case RRef::kMember:
      return Rcpp::as<Rcpp::Environment>(dereference(ref.member().env())).get(ref.member().name());
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
      WithOutputConsumer with(emptyConsumer);
      return RI->evalCode(code, env);
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

Status RPIServiceImpl::copyToPersistentRef(ServerContext* context, const RRef* request, Int32Value* response) {
  executeOnMainThread([&] {
    response->set_value(persistentRefStorage.add(dereference(*request)));
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::disposePersistentRefs(ServerContext*, const PersistentRefList* request, Empty*) {
  std::vector<int> refs(request->indices().begin(), request->indices().end());
  executeOnMainThreadAsync([=] {
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


Status RPIServiceImpl::evaluateAsStringList(ServerContext* context, const RRef* request, StringList* response) {
  executeOnMainThread([&] {
    Rcpp::RObject object = dereference(*request);
    if (!Rcpp::is<Rcpp::GenericVector>(object) && !Rcpp::is<Rcpp::CharacterVector>(object)) {
      if (Rf_inherits(object, "factor")) {
        Rcpp::CharacterVector vector = Rcpp::as<Rcpp::CharacterVector>(object);
        for (auto const& x : vector) {
          response->add_list(Rcpp::as<std::string>(x));
        }
      }
      return;
    }
    Rcpp::GenericVector vector = Rcpp::as<Rcpp::GenericVector>(object);
    for (auto const& x : vector) {
      if (Rcpp::is<std::string>(x) && !Rcpp::as<bool>(RI->isNa(x))) {
        response->add_list(Rcpp::as<std::string>(x));
      } else {
        response->add_list("");
      }
    }
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::loadObjectNames(ServerContext* context, const RRef* request, StringList* response) {
  executeOnMainThread([&] {
    for (const char* x : Rcpp::as<Rcpp::CharacterVector>(RI->ls(dereference(*request)))) {
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
    switch (request->tabletype()) {
      case TableColumnsInfoRequest_TableType_DPLYR:
        if (!Rcpp::as<bool>(RI->inherits(tableObj, "tbl_df"))) return;
        break;
      case TableColumnsInfoRequest_TableType_DATA_TABLE:
        if (!Rcpp::as<bool>(RI->inherits(tableObj, "data.table"))) return;
        break;
      default:;
    }
    Rcpp::DataFrame table = tableObj;
    Rcpp::CharacterVector names = table.names();
    for (int i = 0; i < (int)table.ncol(); ++i) {
      TableColumnsInfo::Column *column = response->add_columns();
      column->set_name(names[i]);
      column->set_type(Rcpp::as<std::string>(RI->paste(RI->classes(table[i]), Rcpp::Named("collapse", ","))));
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
