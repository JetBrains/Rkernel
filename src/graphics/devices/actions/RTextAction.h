#ifndef RWRAPPER_RTEXTACTION_H
#define RWRAPPER_RTEXTACTION_H

#include <string>

#include "RGraphicsAction.h"

namespace devices {
  namespace actions {
    class RTextAction : public RGraphicsAction {
    private:
      std::string text;
      Point at;
      double rot;
      double hadj;  // Have no idea, what this field means
      R_GE_gcontext context;

    public:
      RTextAction(std::string text, Point at, double rot, double hadj, pGEcontext context);

      void rescale(const RescaleInfo& rescaleInfo) override;
      void perform(Ptr<RGraphicsDevice> device) override;
      Ptr<RGraphicsAction> clone() override;
      std::string toString() override;
    };
  }
}

#endif //RWRAPPER_RTEXTACTION_H
