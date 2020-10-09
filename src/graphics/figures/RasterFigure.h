#ifndef RWRAPPER_RASTERFIGURE_H
#define RWRAPPER_RASTERFIGURE_H

#include <sstream>

#include "Figure.h"
#include "../RasterImage.h"
#include "../AffinePoint.h"

namespace graphics {

class RasterFigure : public Figure {
private:
  RasterImage image;
  AffinePoint from;
  AffinePoint to;
  double angle;  // degrees (CCW)
  bool interpolate;

public:
  RasterFigure(RasterImage image, AffinePoint from, AffinePoint to, double angle, bool interpolate)
    : image(std::move(image)), from(from), to(to), angle(angle), interpolate(interpolate) {}

  FigureKind getKind() const override {
    return FigureKind::RASTER;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "RasterFigure(image: " << image << ", from: " << from << ", to: " << to
         << ", angle: " << angle << ", interpolate: " << interpolate << ")";
    return sout.str();
  }

  const RasterImage& getImage() const {
    return image;
  }

  AffinePoint getFrom() const {
    return from;
  }

  AffinePoint getTo() const {
    return to;
  }

  double getAngle() const {
    return angle;
  }

  bool getInterpolate() const {
    return interpolate;
  }
};

}  // graphics

#endif //RWRAPPER_RASTERFIGURE_H
