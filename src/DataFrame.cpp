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
#include "DataFrame.h"

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

bool isSupportedDataFrame(SEXP x) {
  return Rf_isMatrix(x) || isDataFrame(x);
}

static PrSEXP& getDataFrameStorageEnv() {
  static PrSEXP env = RI->newEnv();
  return env;
}

static std::unordered_map<SEXP, DataFrameInfo*> dataFrameCache;

DataFrameInfo::~DataFrameInfo() {
  getDataFrameStorageEnv().assign(std::to_string(index), R_NilValue);
  dataFrameCache.erase(initialDataFrame);
  if (finalizer) finalizer();
}

static void initDataFrame(DataFrameInfo *info) {
  PrSEXP dataFrame = info->initialDataFrame;
  dataFrameCache[info->initialDataFrame] = info;
  getDataFrameStorageEnv().assign(std::to_string(info->index), dataFrame);
  if (Rf_isMatrix(dataFrame)) {
    dataFrame = RI->dataFrame(dataFrame, named("stringsAsFactors", false));
  }
  if (!isDataFrame(dataFrame)) {
    RI->stop("Object is not a valid data frame");
  }

  int ncol = asInt(RI->ncol(dataFrame));
  dataFrame = Rf_duplicate(dataFrame);
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
    // `rownames = NA` keep row names
    dataFrame = RI->tibbleAsTibble(dataFrame, named("rownames", R_NaInt));
  }
  dataFrame = RI->tibbleRowNamesToColumn(dataFrame, ROW_NAMES_COL);
  ShieldSEXP rowNamesColumn = RI->doubleSubscript(dataFrame, ROW_NAMES_COL);
  PrSEXP rowNamesAsNum = RI->strtoi(rowNamesColumn); // As integer (but not numeric)
  if (asBool(RI->any(RI->isNa(rowNamesAsNum)))) {
    rowNamesAsNum = RI->asNumeric(rowNamesColumn); // As numeric
  }
  if (!asBool(RI->any(RI->isNa(rowNamesAsNum)))) {
    dataFrame = RI->doubleSubscriptAssign(dataFrame, ROW_NAMES_COL, named("value", rowNamesAsNum));
  }

  info->dataFrame = dataFrame;
}

static DataFrameInfo *getDataFrameByRef(RRef const* ref) {
  ShieldSEXP ptr = rpiService->dereference(*ref);
  if (ptr.type() != EXTPTRSXP) return nullptr;
  return (DataFrameInfo*)R_ExternalPtrAddr(ptr);
}

static DataFrameInfo *getDataFrameByIndex(int index) {
  if (!rpiService->persistentRefStorage.has(index)) return nullptr;
  ShieldSEXP ptr = rpiService->persistentRefStorage[index];
  if (ptr.type() != EXTPTRSXP) return nullptr;
  return (DataFrameInfo*)R_ExternalPtrAddr(ptr);
}

DataFrameInfo *registerDataFrame(SEXP x, bool isTemporary) {
  SHIELD(x);
  if (!isTemporary && dataFrameCache.count(x)) {
    DataFrameInfo *info = dataFrameCache[x];
    if (info == getDataFrameByIndex(info->index)) return info;
  }
  ShieldSEXP extPtr = rAlloc<DataFrameInfo>();
  DataFrameInfo *info = (DataFrameInfo*)R_ExternalPtrAddr(extPtr);

  info->initialDataFrame = x;
  if (isTemporary) {
    info->dataFrame = info->initialDataFrame;
  } else {
    initDataFrame(info);
  }

  info->index = rpiService->persistentRefStorage.add(extPtr);
  return info;
}

static void createRefresher(DataFrameInfo *info, const RRef* _ref) {
  RRef ref;
  ref.CopyFrom(*_ref);
  RRef* current = &ref;
  bool dereferenceIt = false;
  for (bool done = false; !done;) {
    switch (current->ref_case()) {
    case RRef::kPersistentIndex:
      return;
    case RRef::kGlobalEnv:
      done = true;
      break;
    case RRef::kCurrentEnv:
    case RRef::kSysFrameIndex:
    case RRef::kErrorStackSysFrameIndex:
      dereferenceIt = true;
      done = true;
      break;
    case RRef::kMember:
      current = current->mutable_member()->mutable_env();
      dereferenceIt = true;
      done = true;
      break;
    case RRef::kParentEnv:
      current = current->mutable_parentenv()->mutable_env();
      dereferenceIt = true;
      done = true;
      break;
    case RRef::kExpression:
      current = current->mutable_expression()->mutable_env();
      dereferenceIt = true;
      done = true;
      break;
    case RRef::kListElement:
      current = current->mutable_listelement()->mutable_list();
      break;
    case RRef::kAttributes:
      current = current->mutable_attributes();
      break;
    default:
      return;
    }
  }

  if (dereferenceIt) {
    int index = rpiService->persistentRefStorage.add(rpiService->dereference(*current));
    current->set_persistentindex(index);
    info->finalizer = [=] { rpiService->persistentRefStorage.remove(index); };
  }
  info->refresher = [=] { return rpiService->dereference(ref); };
}

Status RPIServiceImpl::dataFrameRegister(ServerContext* context, const RRef* request, Int32Value* response) {
  response->set_value(-1);
  executeOnMainThread([&] {
    if (!initDplyr()) return;
    PrSEXP dataFrame = dereference(*request);
    DataFrameInfo *info = registerDataFrame(dataFrame);
    createRefresher(info, request);
    response->set_value(info->index);
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
    DataFrameInfo *info = getDataFrameByRef(request);
    if (info == nullptr) return;
    ShieldSEXP dataFrame = info->dataFrame;
    response->set_canrefresh(bool(info->refresher));
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
    DataFrameInfo *info = getDataFrameByRef(&request->ref());
    if (info == nullptr) return;
    ShieldSEXP dataFrame = info->dataFrame;
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
    DataFrameInfo *info = getDataFrameByRef(&request->ref());
    if (info == nullptr) return;
    ShieldSEXP dataFrame = info->dataFrame;
    std::vector<PrSEXP> arrangeArgs = {dataFrame};
    for (auto const& key : request->keys()) {
      if (key.descending()) {
        arrangeArgs.emplace_back(RI->dplyrDesc(RI->doubleSubscript(dataFrame, key.columnindex() + 1)));
      } else {
        arrangeArgs.emplace_back(RI->doubleSubscript(dataFrame, key.columnindex() + 1));
      }
    }
    DataFrameInfo *newInfo = registerDataFrame(invokeFunction(RI->dplyrArrange, arrangeArgs), true);
    response->set_value(newInfo->index);
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
    DataFrameInfo *info = getDataFrameByRef(&request->ref());
    if (info == nullptr) return;
    ShieldSEXP dataFrame = info->dataFrame;
    ShieldSEXP maskWithNa = applyFilter(dataFrame, request->filter());
    ShieldSEXP mask = RI->replace(maskWithNa, RI->isNa(maskWithNa), false);
    PrSEXP call = R_NilValue;
    for (int i = (int)dataFrame.length() - 1; i >= 0; --i) {
      call = Rf_lcons(RI->subscript(dataFrame[i], mask), call);
      SET_TAG(call, Rf_install(std::to_string(i).c_str()));
    }
    ShieldSEXP names = RI->names(dataFrame);
    ShieldSEXP newDataFrame = RI->namesAssign(safeEval(Rf_lcons(RI->dplyrTibble, call), R_GlobalEnv), names);

    DataFrameInfo *newInfo = registerDataFrame(newDataFrame, true);
    response->set_value(newInfo->index);
  }, context, true);
  return Status::OK;
}

Status RPIServiceImpl::dataFrameRefresh(ServerContext* context, const RRef* request, BoolValue* response) {
  executeOnMainThread([&] {
    if (!initDplyr()) return;
    DataFrameInfo *info = getDataFrameByRef(request);
    if (info == nullptr) return;
    if (!info->refresher) return;
    ShieldSEXP newTable = info->refresher();
    if (newTable == info->initialDataFrame || !isSupportedDataFrame(newTable)) return;
    dataFrameCache.erase(info->initialDataFrame);
    info->initialDataFrame = newTable;
  initDataFrame(info);
    response->set_value(true);
  }, context, true);
  return Status::OK;
}
