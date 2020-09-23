#ifndef RWRAPPER_TEXTFIGURE_H
#define RWRAPPER_TEXTFIGURE_H

#include <sstream>

#include "Figure.h"
#include "../AffinePoint.h"

namespace graphics {

class TextFigure : public Figure {
private:
  std::string text;
  AffinePoint position;
  double angle;  // degrees
  double anchor;  // [0.0, 1.0]
  int fontIndex;
  int colorIndex;

public:
  TextFigure(std::string text, AffinePoint position, double angle, double anchor, int fontIndex, int colorIndex)
    : text(std::move(text)), position(position), angle(angle), anchor(anchor), fontIndex(fontIndex), colorIndex(colorIndex) {}

  FigureKind getKind() const override {
    return FigureKind::TEXT;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "TextFigure(text: '" << text << "', position: " << position << ", angle: " << angle
         << ", anchor: " << anchor << ", fontIndex: " << fontIndex << ", colorIndex: " << colorIndex << ")";
    return sout.str();
  }

  const std::string& getText() const {
    return text;
  }

  AffinePoint getPosition() const {
    return position;
  }

  double getAngle() const {
    return angle;
  }

  double getAnchor() const {
    return anchor;
  }

  int getFontIndex() const {
    return fontIndex;
  }

  int getColorIndex() const {
    return colorIndex;
  }
};

}  // graphics

#endif //RWRAPPER_TEXTFIGURE_H
