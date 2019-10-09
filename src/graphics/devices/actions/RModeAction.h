#ifndef RWRAPPER_RMODEACTION_H
#define RWRAPPER_RMODEACTION_H

#include "RGraphicsAction.h"

namespace devices {
  namespace actions {
    class RModeAction : public RGraphicsAction {
    private:
      int mode;

    public:
      explicit RModeAction(int mode);

      void rescale(const RescaleInfo& rescaleInfo) override;
      void perform(Ptr<RGraphicsDevice> device) override;
      Ptr<RGraphicsAction> clone() override;
      std::string toString() override;
    };
  }
}

#endif //RWRAPPER_RMODEACTION_H
