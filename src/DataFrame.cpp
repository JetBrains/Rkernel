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
#include "IO.h"
#include "util/RUtil.h"

const std::string ROW_NAMES_COL = "rwr_rownames_column";

Status RPIServiceImpl::dataFrameRegister(ServerContext* context, const RRef* request, Int32Value* response) {
  response->set_value(-1);
  executeOnMainThread([&] {
    if (!RI->initDplyr()) return;
    Rcpp::RObject obj = dereference(*request);
    if (!Rcpp::is<Rcpp::DataFrame>(obj)) return;
    Rcpp::DataFrame dataFrame = Rcpp::clone(Rcpp::as<Rcpp::DataFrame>(obj));
    Rcpp::RObject namesObj = dataFrame.names();
    Rcpp::CharacterVector namesList = Rcpp::is<Rcpp::CharacterVector>(namesObj) ?
        Rcpp::as<Rcpp::CharacterVector>(namesObj) : Rcpp::CharacterVector();
    int ncol = dataFrame.ncol();
    Rcpp::CharacterVector names(ncol);
    for (int i = 0; i < ncol; ++i) {
      names[i] = i < namesList.size() && !namesList.is_na(namesList[i]) ? namesList[i] : "";
      if (names[i].empty()) names[i] = "Column " + std::to_string(i + 1);
    }
    dataFrame.names() = names;
    if (RI->dplyr->isTbl(dataFrame)) {
      dataFrame = RI->dplyr->ungroup(dataFrame);
    } else {
      if (!RI->isDataFrame(dataFrame)) {
        dataFrame = RI->dataFrame(dataFrame, Rcpp::Named("stringsAsFactors", false));
      }
      dataFrame = RI->dplyr->asTbl(dataFrame);
    }
    dataFrame = RI->dplyr->addRowNames(dataFrame, ROW_NAMES_COL);
    if (!Rcpp::as<bool>(RI->any(RI->isNa(RI->asInteger(dataFrame[ROW_NAMES_COL]))))) {
      dataFrame[ROW_NAMES_COL] = RI->asInteger(dataFrame[ROW_NAMES_COL]);
    }
    for (int index : dataFramesCache) {
      if (Rcpp::as<bool>(RI->identical(dataFrame, persistentRefStorage[index]))) {
        response->set_value(index);
        return;
      }
    }
    response->set_value(persistentRefStorage.add(dataFrame));
    dataFramesCache.insert(response->value());
  }, context);
  return Status::OK;
}

static std::string getClasses(Rcpp::RObject const& obj) {
  return Rcpp::as<std::string>(RI->paste(RI->classes(obj), Rcpp::Named("collapse", ",")));
}

Status RPIServiceImpl::dataFrameGetInfo(ServerContext* context, const RRef* request, DataFrameInfoResponse* response) {
  executeOnMainThread([&] {
    if (!RI->initDplyr()) return;
    Rcpp::DataFrame dataFrame = dereference(*request);
    response->set_nrows(dataFrame.nrow());
    Rcpp::CharacterVector names = RI->names(dataFrame);
    for (int i = 0; i < (int)dataFrame.ncol(); ++i) {
      DataFrameInfoResponse::Column *columnInfo = response->add_columns();
      Rcpp::RObject column = RI->doubleSubscript(dataFrame, i + 1);
      std::string cls = getClasses(column);
      if (names[i] == ROW_NAMES_COL) {
        columnInfo->set_isrownames(true);
      } else {
        columnInfo->set_name(names[i]);
      }
      if (cls == "integer") {
        columnInfo->set_type(DataFrameInfoResponse::INTEGER);
        columnInfo->set_sortable(true);
      } else if (cls == "numeric") {
        columnInfo->set_type(DataFrameInfoResponse::DOUBLE);
        columnInfo->set_sortable(true);
      } else if (cls == "logical") {
        columnInfo->set_type(DataFrameInfoResponse::BOOLEAN);
        columnInfo->set_sortable(true);
      } else {
        columnInfo->set_type(DataFrameInfoResponse::STRING);
        columnInfo->set_sortable(cls == "character" || Rf_inherits(column, "factor"));
      }
    }
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::dataFrameGetData(ServerContext* context, const DataFrameGetDataRequest* request, DataFrameGetDataResponse* response) {
  executeOnMainThread([&] {
    if (!RI->initDplyr()) return;
    Rcpp::DataFrame dataFrame = dereference(request->ref());
    int start = request->start();
    int end = request->end();
    for (int i = 0; i < (int)dataFrame.ncol(); ++i) {
      DataFrameGetDataResponse::Column* columnProto = response->add_columns();
      Rcpp::RObject columnObj = RI->subscript(Rcpp::Shield<SEXP>(RI->doubleSubscript(dataFrame, i + 1)),
                                              Rcpp::Shield<SEXP>(RI->colon(start + 1, end)));
      std::string cls = getClasses(columnObj);
      Rcpp::GenericVector column = Rcpp::as<Rcpp::GenericVector>(columnObj);
      Rcpp::LogicalVector isNa = RI->isNa(columnObj);
      if (cls == "integer") {
        for (int j = 0; j < (int)column.size(); ++j) {
          if (isNa[j]) {
            columnProto->add_values()->mutable_na();
          } else {
            columnProto->add_values()->set_intvalue(column[j]);
          }
        }
      } else if (cls == "numeric") {
        for (int j = 0; j < (int)column.size(); ++j) {
          if (isNa[j]) {
            columnProto->add_values()->mutable_na();
          } else {
            columnProto->add_values()->set_doublevalue(column[j]);
          }
        }
      } else if (cls == "logical") {
        for (int j = 0; j < (int)column.size(); ++j) {
          if (isNa[j]) {
            columnProto->add_values()->mutable_na();
          } else {
            columnProto->add_values()->set_booleanvalue(column[j]);
          }
        }
      } else {
        for (int j = 0; j < (int)column.size(); ++j) {
          if (isNa[j]) {
            columnProto->add_values()->mutable_na();
          } else {
            columnProto->add_values()->set_stringvalue(
              translateToUTF8(RI->paste(column[j], Rcpp::Named("collapse", "; "))));
          }
        }
      }
    }
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::dataFrameSort(ServerContext* context, const DataFrameSortRequest* request, Int32Value* response) {
  response->set_value(-1);
  executeOnMainThread([&] {
    if (!RI->initDplyr()) return;
    Rcpp::RObject dataFrame = dereference(request->ref());
    std::vector<SEXP> arrangeArgs = {dataFrame};
    for (auto const& key : request->keys()) {
      if (key.descending()) {
        arrangeArgs.push_back(RI->dplyr->desc(RI->doubleSubscript(dataFrame, key.columnindex() + 1)));
      } else {
        arrangeArgs.push_back(RI->doubleSubscript(dataFrame, key.columnindex() + 1));
      }
    }
    response->set_value(persistentRefStorage.add(invokeFunction(RI->dplyr->arrange, arrangeArgs)));
  }, context);
  return Status::OK;
}

static Rcpp::LogicalVector applyFilter(Rcpp::DataFrame const& df, DataFrameFilterRequest::Filter const& filter) {
  if (filter.has_true_()) {
    return Rcpp::LogicalVector(df.nrows(), true);
  }
  if (filter.has_composed()) {
    switch (filter.composed().type()) {
      case DataFrameFilterRequest_Filter_ComposedFilter_Type_AND: {
        Rcpp::LogicalVector result(df.nrows(), true);
        for (auto const& f : filter.composed().filters()) {
          result = result & applyFilter(df, f);
        }
        return result;
      }
      case DataFrameFilterRequest_Filter_ComposedFilter_Type_OR: {
        Rcpp::LogicalVector result(df.nrows(), false);
        for (auto const& f : filter.composed().filters()) {
          result = result | applyFilter(df, f);
        }
        return result;
      }
      case DataFrameFilterRequest_Filter_ComposedFilter_Type_NOT: {
        Rcpp::LogicalVector result(df.nrows(), true);
        for (auto const& f : filter.composed().filters()) {
          result = result & applyFilter(df, f);
        }
        return !result;
      }
      default:
        return Rcpp::LogicalVector(df.nrows(), true);
    }
  }
  if (filter.has_operator_()) {
    Rcpp::RObject column = RI->doubleSubscript(df, filter.operator_().column() + 1);
    std::string cls = getClasses(column);
    std::string const& strValue = filter.operator_().value();
    Rcpp::RObject value;
    if (cls == "integer") {
      value = RI->asInteger(strValue);
    } else if (cls == "numeric") {
      value = RI->asDouble(strValue);
    } else if (cls == "logical") {
      value = RI->asLogical(strValue);
    } else {
      value = strValue;
    }
    switch (filter.operator_().type()) {
      case DataFrameFilterRequest_Filter_Operator_Type_EQ:
        return RI->eq(column, value);
      case DataFrameFilterRequest_Filter_Operator_Type_NEQ:
        return RI->neq(column, value);
      case DataFrameFilterRequest_Filter_Operator_Type_LESS:
        return RI->less(column, value);
      case DataFrameFilterRequest_Filter_Operator_Type_GREATER:
        return RI->greater(column, value);
      case DataFrameFilterRequest_Filter_Operator_Type_LEQ:
        return RI->leq(column, value);
      case DataFrameFilterRequest_Filter_Operator_Type_GEQ:
        return RI->geq(column, value);
      case DataFrameFilterRequest_Filter_Operator_Type_REGEX:
        try {
          return RI->grepl(strValue, column);
        } catch (Rcpp::eval_error const&) {
        }
      default:
        return Rcpp::LogicalVector(df.nrows(), true);
    }
  }
  if (filter.has_nafilter()) {
    Rcpp::RObject column = RI->doubleSubscript(df, filter.nafilter().column() + 1);
    if (filter.nafilter().isna()) {
      return RI->isNa(column);
    } else {
      return !Rcpp::as<Rcpp::LogicalVector>(RI->isNa(column));
    }
  }
  return Rcpp::LogicalVector(df.nrows(), true);
}

Status RPIServiceImpl::dataFrameFilter(ServerContext* context, const DataFrameFilterRequest* request, Int32Value* response) {
  response->set_value(-1);
  executeOnMainThread([&] {
    if (!RI->initDplyr()) return;
    Rcpp::DataFrame dataFrame = dereference(request->ref());
    int index = persistentRefStorage.add(RI->dplyr->filter(dataFrame, applyFilter(dataFrame, request->filter())));
    response->set_value(index);
  }, context);
  return Status::OK;
}

Status RPIServiceImpl::dataFrameDispose(ServerContext*, const Int32Value* request, Empty*) {
  executeOnMainThread([&] {
    dataFramesCache.erase(request->value());
  });
  return Status::OK;
}
