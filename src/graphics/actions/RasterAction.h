#ifndef RWRAPPER_RASTERACTION_H
#define RWRAPPER_RASTERACTION_H

#include <sstream>

#include "Action.h"
#include "../RasterImage.h"
#include "../Rectangle.h"

namespace graphics {

class RasterAction : public Action {
private:
  RasterImage image;
  Rectangle rectangle;  // inches
  double angle;  // degrees (CCW)
  bool interpolate;

public:
  RasterAction(RasterImage image, Rectangle rectangle, double angle, bool interpolate)
    : image(std::move(image)), rectangle(rectangle), angle(angle), interpolate(interpolate) {}

  ActionKind getKind() const override {
    return ActionKind::RASTER;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "RasterAction(image: " << image << ", rectangle: " << rectangle
         << ", angle: " << angle << ", interpolate: " << interpolate << ")";
    return sout.str();
  }

  const RasterImage& getImage() const {
    return image;
  }

  Rectangle getRectangle() const {
    return rectangle;
  }

  double getAngle() const {
    return angle;
  }

  bool getInterpolate() const {
    return interpolate;
  }
};

}  // graphics

#endif //RWRAPPER_RASTERACTION_H
