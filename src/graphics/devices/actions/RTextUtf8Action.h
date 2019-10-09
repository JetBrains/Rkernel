#ifndef RWRAPPER_RTEXTUTF8ACTION_H
#define RWRAPPER_RTEXTUTF8ACTION_H

#include "RGraphicsAction.h"
#include "RTextAction.h"

namespace devices {
  namespace actions {
    class RTextUtf8Action : public RGraphicsAction {
    private:
      std::string text;
      Point at;
      double rot;
      double hadj;  // Have no idea, what this field means
      R_GE_gcontext context;

    public:
      RTextUtf8Action(std::string text, Point at, double rot, double hadj, pGEcontext context);

      void rescale(const RescaleInfo& rescaleInfo) override;
      void perform(Ptr<RGraphicsDevice> device) override;
      Ptr<RGraphicsAction> clone() override;
      std::string toString() override;
    };
  }
}

#endif //RWRAPPER_RTEXTUTF8ACTION_H
