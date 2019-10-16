#ifndef RWRAPPER_RLAZYGRAPHICSDEVICE_H
#define RWRAPPER_RLAZYGRAPHICSDEVICE_H

#include <string>
#include <vector>

#include "RGraphicsDevice.h"
#include "actions/RGraphicsAction.h"

namespace devices {
  class RLazyGraphicsDevice : public RGraphicsDevice {
  private:
    // Note: might be extended in future
    enum class Status {
      NORMAL,
      LARGE_MARGINS,
    };

    using ActionList = std::vector<Ptr<actions::RGraphicsAction>>;

    ActionList actions;
    ActionList previousActions;
    std::string snapshotDirectory;
    int snapshotNumber;
    int snapshotVersion;
    ScreenParameters parameters;
    Ptr<RGraphicsDevice> slave;
    Rectangle artBoard;
    Status status;

    ActionList copyActions();
    void applyActions(const ActionList& actionList);
    Ptr<RGraphicsDevice> getSlave(const char* typeSuffix = nullptr);
    std::string buildSnapshotPath(const char* typeSuffix, const char* errorSuffix = nullptr);
    Ptr<RGraphicsDevice> initializeSlaveDevice(const std::string& path);
    void shutdownSlaveDevice();
    static Rectangle buildCanvas(double width, double height);
    Rectangle buildCurrentCanvas();

    RLazyGraphicsDevice(ActionList previousActions, std::string snapshotDirectory, int snapshotNumber, ScreenParameters parameters);

  public:
    RLazyGraphicsDevice(std::string snapshotDirectory, int snapshotNumber, ScreenParameters parameters);

    void drawCircle(Point center, double radius, pGEcontext context) override;
    void clip(Point from, Point to) override;
    void close() override;
    void drawLine(Point from, Point to, pGEcontext context) override;
    MetricInfo metricInfo(int character, pGEcontext context) override;
    void setMode(int mode) override;
    void newPage(pGEcontext context) override;
    void drawPolygon(const std::vector<Point> &points, pGEcontext context) override;
    void drawPolyline(const std::vector<Point> &points, pGEcontext context) override;
    void drawRect(Point from, Point to, pGEcontext context) override;
    void drawPath(const std::vector<Point> &points, const std::vector<int> &numPointsPerPolygon, Rboolean winding, pGEcontext context) override;
    void drawRaster(const RasterInfo &rasterInfo, Point at, double width, double height, double rotation, Rboolean interpolate, pGEcontext context) override;
    ScreenParameters screenParameters() override;
    double widthOfStringUtf8(const char* text, pGEcontext context) override;
    void drawTextUtf8(const char* text, Point at, double rotation, double heightAdjustment, pGEcontext context) override;
    bool dump(SnapshotType type) override;
    void rescale(double newWidth, double newHeight) override;
    Ptr<devices::RGraphicsDevice> clone() override;
    bool isBlank() override;
  };
}

#endif //RWRAPPER_RLAZYGRAPHICSDEVICE_H
