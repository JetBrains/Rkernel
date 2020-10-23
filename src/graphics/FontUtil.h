#ifndef RWRAPPER_FONTUTIL_H
#define RWRAPPER_FONTUTIL_H

#include <string>

namespace graphics {

class FontUtil {
public:
  FontUtil() = delete;

  static std::string matchName(const std::string& alias);
};

}

#endif //RWRAPPER_FONTUTIL_H
