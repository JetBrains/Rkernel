#ifndef RWRAPPER_RASTERIMAGE_H
#define RWRAPPER_RASTERIMAGE_H

#include <cstdint>
#include <iostream>

#include "Ptr.h"

namespace graphics {

struct RasterImage {
  int width;  // pixels
  int height;  // pixels
  Ptr<uint8_t> data;   // little-endian uint32[] of ARGB
};

inline std::ostream& operator<<(std::ostream& out, const RasterImage& raster) {
  out << "RasterImage(width: " << raster.width << ", height: " << raster.height << ")";
  return out;
}

}  // graphics

#endif //RWRAPPER_RASTERIMAGE_H
