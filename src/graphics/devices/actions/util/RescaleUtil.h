#ifndef RWRAPPER_RESCALEUTIL_H
#define RWRAPPER_RESCALEUTIL_H

#include "../RGraphicsAction.h"

namespace devices {
  namespace actions {
    namespace util {
      Point rescale(Point point, const RescaleInfo& rescaleInfo);
      void rescaleInPlace(Point& point, const RescaleInfo& rescaleInfo);
    }
  }
}

#endif //RWRAPPER_RESCALEUTIL_H
