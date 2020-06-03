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
#include "IO.h"
#include "RStuff/RUtil.h"

const char* ROW_NAMES_COL = "rwr_rownames_column";

static bool initDplyr() {
  try {
    RI->loadNamespace("dplyr");
    return true;
  } catch (RError const& e) {
    std::cerr << "Failed to load dplyr: " << e.what() << "\n";
    return false;
  }
}

Status RPIServiceImpl::dataFrameRegister(ServerContext* context, const RRef* request, Int32Value* response) {
  response->set_value(-1);
  executeOnMainThread([&] {
    if (!initDplyr()) return;
    PrSEXP dataFrame = dereference(*request);
    if (Rf_isMatrix(dataFrame)) {
      dataFrame = RI->dataFrame(dataFrame, named("stringsAsFactors", false));
    }
    if (!isDataFrame(dataFrame)) return;
    dataFrame = Rf_duplicate(dataFrame);
    int ncol = asInt(RI->ncol(dataFrame));
    for (int i = 0; i < ncol; ++i) {
      if (Rf_inherits(dataFrame[i], "POSIXlt")) {
        SET_VECTOR_ELT(dataFrame, i, RI->asPOSIXct(dataFrame[i]));
      }
    }
    ShieldSEXP namesList = RI->names(dataFrame);
    std::vector<std::string> names(ncol);
    for (int i = 0; i < ncol; ++i) {
      names[i] = i < namesList.length() && !namesList.isNA(i) ? stringEltUTF8(namesList, i) : "";
      if (names[i].empty()) names[i] = "Column " + std::to_string(i + 1);
    }
    dataFrame = RI->namesAssign(dataFrame, names);
    if (asBool(RI->dplyrIsTbl(dataFrame))) {
      dataFrame = RI->dplyrUngroup(dataFrame);
    } else {
      if (!asBool(RI->isDataFrame(dataFrame))) {
        dataFrame = RI->dataFrame(dataFrame, named("stringsAsFactors", false));
      }
      dataFrame = RI->dplyrAsTbl(dataFrame);
    }
    dataFrame = RI->dplyrAddRowNames(dataFrame, ROW_NAMES_COL);
    ShieldSEXP rowNamesAsInt = RI->asInteger(RI->doubleSubscript(dataFrame, ROW_NAMES_COL));
    if (!asBool(RI->any(RI->isNa(rowNamesAsInt)))) {
      dataFrame = RI->doubleSubscriptAssign(dataFrame, ROW_NAMES_COL, named("value", rowNamesAsInt));
    }
    for (int index : dataFramesCache) {
      if (asBool(RI->identical(dataFrame, persistentRefStorage[index]))) {
        response->set_value(index);
        return;
      }
    }
    response->set_value(persistentRefStorage.add(dataFrame));
    dataFramesCache.insert(response->value());
  }, context, true);
  return Status::OK;
}

static std::string getClasses(SEXP obj) {
  SHIELD(obj);
  return asStringUTF8(RI->paste(RI->classes(obj), named("collapse", ",")));
}

Status RPIServiceImpl::dataFrameGetInfo(ServerContext* context, const RRef* request, DataFrameInfoResponse* response) {
  executeOnMainThread([&] {
    if (!initDplyr()) return;
    ShieldSEXP dataFrame = dereference(*request);
    response->set_nrows(asInt(RI->nrow(dataFrame)));
    ShieldSEXP names = RI->names(dataFrame);
    int ncol = asInt(RI->ncol(dataFrame));
    for (int i = 0; i < ncol; ++i) {
      DataFrameInfoResponse::Column *columnInfo = response->add_columns();
      ShieldSEXP column = RI->doubleSubscript(dataFrame, i + 1);
      std::string cls = getClasses(column);
      const char* name = stringEltUTF8(names, i);
      if (!strcmp(name, ROW_NAMES_COL)) {
        columnInfo->set_isrownames(true);
      } else {
        columnInfo->set_name(name);
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
        columnInfo->set_sortable(cls == "character" || Rf_inherits(column, "factor") ||
                                 Rf_inherits(column, "POSIXt"));
      }
    }
  }, context, true);
  return Status::OK;
}

Status RPIServiceImpl::dataFrameGetData(ServerContext* context, const DataFrameGetDataRequest* request, DataFrameGetDataResponse* response) {
  executeOnMainThread([&] {
    if (!initDplyr()) return;
    ShieldSEXP dataFrame = dereference(request->ref());
    int start = request->start();
    int end = request->end();
    int ncol = asInt(RI->ncol(dataFrame));
    for (int i = 0; i < ncol; ++i) {
      DataFrameGetDataResponse::Column* columnProto = response->add_columns();
      ShieldSEXP wholeColumn = RI->doubleSubscript(dataFrame, i + 1);
      ShieldSEXP column = RI->subscript(wholeColumn, RI->colon(start + 1, end));
      std::string cls = getClasses(column);
      if (cls == "integer") {
        for (int j = 0; j < column.length(); ++j) {
          if (column.isNA(j)) {
            columnProto->add_values()->mutable_na();
          } else {
            columnProto->add_values()->set_intvalue(asInt(column[j]));
          }
        }
      } else if (cls == "numeric") {
        for (int j = 0; j < column.length(); ++j) {
          if (column.isNA(j)) {
            columnProto->add_values()->mutable_na();
          } else {
            columnProto->add_values()->set_doublevalue(asDouble(column[j]));
          }
        }
      } else if (cls == "logical") {
        for (int j = 0; j < column.length(); ++j) {
          if (column.isNA(j)) {
            columnProto->add_values()->mutable_na();
          } else {
            columnProto->add_values()->set_booleanvalue(asBool(column[j]));
          }
        }
      } else {
        for (int j = 0; j < column.length(); ++j) {
          if (column.isNA(j)) {
            columnProto->add_values()->mutable_na();
          } else {
            columnProto->add_values()->set_stringvalue(
              asStringUTF8(RI->paste(RI->doubleSubscript(column, j + 1), named("collapse", "; "))));
          }
        }
      }
    }
  }, context, true);
  return Status::OK;
}

Status RPIServiceImpl::dataFrameSort(ServerContext* context, const DataFrameSortRequest* request, Int32Value* response) {
  response->set_value(-1);
  executeOnMainThread([&] {
    if (!initDplyr()) return;
    ShieldSEXP dataFrame = dereference(request->ref());
    std::vector<PrSEXP> arrangeArgs = {dataFrame};
    for (auto const& key : request->keys()) {
      if (key.descending()) {
        arrangeArgs.emplace_back(RI->dplyrDesc(RI->doubleSubscript(dataFrame, key.columnindex() + 1)));
      } else {
        arrangeArgs.emplace_back(RI->doubleSubscript(dataFrame, key.columnindex() + 1));
      }
    }
    response->set_value(persistentRefStorage.add(invokeFunction(RI->dplyrArrange, arrangeArgs)));
  }, context, true);
  return Status::OK;
}

static SEXP applyFilter(SEXP df, DataFrameFilterRequest::Filter const& filter) {
  SHIELD(df);
  int nrow = asInt(RI->nrow(df));
  if (filter.has_true_()) {
    return RI->rep(true, nrow);
  }
  if (filter.has_composed()) {
    switch (filter.composed().type()) {
      case DataFrameFilterRequest_Filter_ComposedFilter_Type_AND: {
        PrSEXP result = RI->rep(true, nrow);
        for (auto const& f : filter.composed().filters()) {
          result = RI->vectorAnd(result, applyFilter(df, f));
        }
        return result;
      }
      case DataFrameFilterRequest_Filter_ComposedFilter_Type_OR: {
        PrSEXP result = RI->rep(false, nrow);
        for (auto const& f : filter.composed().filters()) {
          result = RI->vectorOr(result, applyFilter(df, f));
        }
        return result;
      }
      case DataFrameFilterRequest_Filter_ComposedFilter_Type_NOT: {
        PrSEXP result = RI->rep(true, nrow);
        for (auto const& f : filter.composed().filters()) {
          result = RI->vectorAnd(result, applyFilter(df, f));
        }
        return RI->vectorNot(result);
      }
      default:
        return RI->rep(true, nrow);
    }
  }
  if (filter.has_operator_()) {
    ShieldSEXP column = RI->doubleSubscript(df, filter.operator_().column() + 1);
    std::string cls = getClasses(column);
    std::string const& strValue = filter.operator_().value();
    PrSEXP value;
    if (cls == "integer") {
      value = RI->asInteger(strValue);
    } else if (cls == "numeric") {
      value = RI->asDouble(strValue);
    } else if (cls == "logical") {
      value = RI->asLogical(strValue);
    } else {
      value = toSEXP(strValue);
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
        } catch (RError const&) {
        }
      default:
        return RI->rep(true, nrow);
    }
  }
  if (filter.has_nafilter()) {
    ShieldSEXP column = RI->doubleSubscript(df, filter.nafilter().column() + 1);
    if (filter.nafilter().isna()) {
      return RI->isNa(column);
    } else {
      return RI->vectorNot(RI->isNa(column));
    }
  }
  return RI->rep(true, nrow);
}

Status RPIServiceImpl::dataFrameFilter(ServerContext* context, const DataFrameFilterRequest* request, Int32Value* response) {
  response->set_value(-1);
  executeOnMainThread([&] {
    if (!initDplyr()) return;
    ShieldSEXP dataFrame = dereference(request->ref());
    int index = persistentRefStorage.add(RI->dplyrFilter(dataFrame, applyFilter(dataFrame, request->filter())));
    response->set_value(index);
  }, context, true);
  return Status::OK;
}

Status RPIServiceImpl::dataFrameDispose(ServerContext*, const Int32Value* request, Empty*) {
  executeOnMainThread([&] {
    dataFramesCache.erase(request->value());
  }, nullptr, true);
  return Status::OK;
}
