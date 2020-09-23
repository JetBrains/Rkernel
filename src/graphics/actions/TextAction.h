#ifndef RWRAPPER_TEXTACTION_H
#define RWRAPPER_TEXTACTION_H

#include <sstream>

#include "Action.h"
#include "../Font.h"
#include "../Point.h"
#include "../Color.h"

namespace graphics {

class TextAction : public Action {
private:
  std::string text;
  Point position;  // inches
  double angle;  // degrees
  double anchor;  // [0.0, 1.0]
  Font font;
  Color color;

public:
  TextAction(std::string text, Point position, double angle, double anchor, Font font, Color color)
    : text(std::move(text)), position(position), angle(angle), anchor(anchor),
      font(std::move(font)), color(color) {}

  ActionKind getKind() const override {
    return ActionKind::TEXT;
  }

  std::string toString() const override {
    auto sout = std::ostringstream();
    sout << "TextAction(text: '" << text << "', position: " << position << ", angle: " << angle
         << ", anchor: " << anchor << ", font: " << font << ", color: " << color << ")";
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

  const Font& getFont() const {
    return font;
  }

  Color getColor() const {
    return color;
  }
};

}  // graphics

#endif //RWRAPPER_TEXTACTION_H
