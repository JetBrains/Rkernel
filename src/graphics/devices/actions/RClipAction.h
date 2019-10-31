#ifndef RWRAPPER_RCLIPACTION_H
#define RWRAPPER_RCLIPACTION_H

#include "RGraphicsAction.h"

namespace graphics {
namespace devices {
namespace actions {

class RClipAction : public RGraphicsAction {
private:
  Point from;
  Point to;

public:
  RClipAction(Point from, Point to);

  void rescale(const RescaleInfo& rescaleInfo) override;
  void perform(Ptr<RGraphicsDevice> device) override;
  Ptr<RGraphicsAction> clone() override;
  std::string toString() override;
  bool isVisible() override;
};

}  // actions
}  // devices
}  // graphics

#endif //RWRAPPER_RCLIPACTION_H
