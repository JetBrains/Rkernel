#ifndef RWRAPPER_TEXTACTION_H
#define RWRAPPER_TEXTACTION_H

#include <sstream>

#include "Action.h"
#include "../Point.h"

namespace graphics {

class TextAction : public Action {
private:
  std::string text;
  Point position;  // inches
  double angle;  // degrees
  double anchor;  // [0.0, 1.0]
  std::string fontName;  // may be empty
  double fontSize;  // inches (multiple by 72 to get points)
  Color color;

public:
  TextAction(std::string text, Point position, double angle, double anchor, std::string fontName, double fontSize, Color color)
    : text(std::move(text)), position(position), angle(angle), anchor(anchor),
      fontName(std::move(fontName)), fontSize(fontSize), color(color) {}

  ActionKind getKind() const override {
    return ActionKind::TEXT;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "TextAction(text: '" << text << "', position: " << position << ", angle: " << angle
         << ", anchor: " << anchor << ", fontName: '" << fontName << "'"
         << ", fontSize: " << fontSize << ", color: " << color << ")";
    return sout.str();
  }

  const std::string& getText() const {
    return text;
  }

  Point getPosition() const {
    return position;
  }

  double getAngle() const {
    return angle;
  }

  double getAnchor() const {
    return anchor;
  }

  const std::string& getFontName() const {
    return fontName;
  }

  double getFontSize() const {
    return fontSize;
  }

  Color getColor() const {
    return color;
  }
};

}  // graphics

#endif //RWRAPPER_TEXTACTION_H
