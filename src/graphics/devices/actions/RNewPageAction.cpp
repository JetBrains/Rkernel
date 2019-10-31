#include "RNewPageAction.h"

namespace graphics {
namespace devices {
namespace actions {

RNewPageAction::RNewPageAction(pGEcontext context) : context(*context) { DEVICE_TRACE; }

void RNewPageAction::rescale(const RescaleInfo &rescaleInfo) {
  DEVICE_TRACE;
  // Nothing to do here
}

void RNewPageAction::perform(Ptr<RGraphicsDevice> device) {
  DEVICE_TRACE;
  device->newPage(&context);
}

Ptr<RGraphicsAction> RNewPageAction::clone() {
  return makePtr<RNewPageAction>(&context);
}

std::string RNewPageAction::toString() {
  return "RNewPageAction {}";
}

bool RNewPageAction::isVisible() {
  return false;
}

}  // actions
}  // devices
}  // graphics
