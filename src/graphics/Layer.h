#ifndef RWRAPPER_LAYER_H
#define RWRAPPER_LAYER_H

#include <vector>

#include "Ptr.h"
#include "figures/Figure.h"

namespace graphics {

struct Layer {
  int viewportIndex;
  int clippingAreaIndex;
  std::vector<Ptr<Figure>> figures;
  bool isAxisText;
};

}  // graphics

#endif //RWRAPPER_LAYER_H
