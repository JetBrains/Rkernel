#ifndef RWRAPPER_PLOT_H
#define RWRAPPER_PLOT_H

#include <vector>

#include "Font.h"
#include "Color.h"
#include "Layer.h"
#include "Stroke.h"
#include "viewports/Viewport.h"

namespace graphics {

enum class PlotError {
  NONE,
  UNKNOWN,
  TOO_COMPLEX,
  GROWING_TEXT,
  UNSUPPORTED_ACTION,
  MISMATCHING_ACTIONS,
};

struct Plot {
  std::vector<Font> fonts;
  std::vector<Color> colors;
  std::vector<Stroke> strokes;
  std::vector<Ptr<Viewport>> viewports;
  std::vector<Layer> layers;
  PlotError error;
};

}  // graphics

#endif //RWRAPPER_PLOT_H
