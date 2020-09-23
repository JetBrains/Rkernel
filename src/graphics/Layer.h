#ifndef RWRAPPER_LAYER_H
#define RWRAPPER_LAYER_H

#include <vector>

#include "Ptr.h"
#include "figures/Figure.h"

namespace graphics {

struct Layer {
  int viewportIndex;
  std::vector<Ptr<Figure>> figures;
};

}  // graphics

#endif //RWRAPPER_LAYER_H
