#include "FontUtil.h"

#include <unordered_map>

namespace graphics {

namespace {

#if defined _WIN32
const auto SANS_NAME = "Arial";
const auto SERIF_NAME = "Times New Roman";
const auto MONO_NAME = "Courier New";
const auto SYMBOL_NAME = "Symbol";
#elif defined __APPLE__
const auto SANS_NAME = "Arial";
const auto SERIF_NAME = "Times";
const auto MONO_NAME = "Courier";
const auto SYMBOL_NAME = "Symbol";
#else
const auto SANS_NAME = "Liberation Sans";
const auto SERIF_NAME = "Liberation Serif";
const auto MONO_NAME = "Liberation Mono";
const auto SYMBOL_NAME = "OpenSymbol";
#endif

const auto SANS_ALIAS = "sans";
const auto SERIF_ALIAS = "serif";
const auto MONO_ALIAS = "mono";
const auto SYMBOL_ALIAS = "symbol";

std::unordered_map<std::string, std::string> createAliasMapping() {
  return std::unordered_map<std::string, std::string> {
      {"", SANS_NAME},
      {SANS_ALIAS, SANS_NAME},
      {SERIF_ALIAS, SERIF_NAME},
      {MONO_ALIAS, MONO_NAME},
      {SYMBOL_ALIAS, SYMBOL_NAME},
  };
}

}  // anonymous

std::string FontUtil::matchName(const std::string &alias) {
  static auto alias2Names = createAliasMapping();
  auto it = alias2Names.find(alias);
  if (it != alias2Names.end()) {
    return it->second;
  } else {
    return alias;
  }
}

}  // graphics
