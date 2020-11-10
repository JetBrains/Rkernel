#ifndef RWRAPPER_PLOTUTIL_H
#define RWRAPPER_PLOTUTIL_H

#include <vector>

#include "Ptr.h"
#include "Plot.h"
#include "ScreenParameters.h"
#include "actions/Action.h"

namespace graphics {

class PlotUtil {
public:
  PlotUtil() = delete;

  static Plot createPlotWithError(PlotError error);
  static Plot extrapolate(/* inches */ Size firstSize, const std::vector<Ptr<Action>>& firstActions,
                          /* inches */ Size secondSize, const std::vector<Ptr<Action>>& secondActions,
                          int totalComplexity);
};

}  // graphics

#endif //RWRAPPER_PLOTUTIL_H
