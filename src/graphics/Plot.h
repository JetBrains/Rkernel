#ifndef RWRAPPER_PLOT_H
#define RWRAPPER_PLOT_H

#include <vector>

#include "Font.h"
#include "Color.h"
#include "Layer.h"
#include "Stroke.h"
#include "Viewport.h"

namespace graphics {

struct Plot {
  std::vector<Font> fonts;
  std::vector<Color> colors;
  std::vector<Stroke> strokes;
  std::vector<Viewport> viewports;
  std::vector<Layer> layers;
};

}  // graphics

#endif //RWRAPPER_PLOT_H
