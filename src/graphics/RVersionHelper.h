//
// Created by user on 15.05.21.
//

#ifndef RWRAPPER_RVERSIONHELPER_H
#define RWRAPPER_RVERSIONHELPER_H

// Replica of the struct _DevDesc from R. Used to estimate the size of the struct in version 4.1
struct SampleDevDesc {
  double left;
  double right;
  double bottom;
  double top;
  double clipLeft;
  double clipRight;
  double clipBottom;
  double clipTop;
  double xCharOffset;
  double yCharOffset;
  double yLineBias;
  double ipr[2];
  double cra[2];
  double gamma;
  Rboolean canClip;
  Rboolean canChangeGamma;
  int canHAdj;
  double startps;
  int startcol;
  int startfill;
  int startlty;
  int startfont;
  double startgamma;
  void *deviceSpecific;
  Rboolean displayListOn;
  Rboolean canGenMouseDown;
  Rboolean canGenMouseMove;
  Rboolean canGenMouseUp;
  Rboolean canGenKeybd;
  Rboolean canGenIdle;
  Rboolean gettingEvent;
  void (*activate)(const pDevDesc);
  void (*circle)(double x, double y, double r, const pGEcontext gc, pDevDesc dd);
  void (*clip)(double x0, double x1, double y0, double y1, pDevDesc dd);
  void (*close)(pDevDesc dd);
  void (*deactivate)(pDevDesc);
  Rboolean (*locator)(double *x, double *y, pDevDesc dd);
  void (*line)(double x1, double y1, double x2, double y2, const pGEcontext gc, pDevDesc dd);
  void (*metricInfo)(int c, const pGEcontext gc, double *ascent, double *descent, double *width, pDevDesc dd);
  void (*mode)(int mode, pDevDesc dd);
  void (*newPage)(const pGEcontext gc, pDevDesc dd);
  void (*polygon)(int n, double *x, double *y, const pGEcontext gc, pDevDesc dd);
  void (*polyline)(int n, double *x, double *y, const pGEcontext gc, pDevDesc dd);
  void (*rect)(double x0, double y0, double x1, double y1, const pGEcontext gc, pDevDesc dd);
  void (*path)(double *x, double *y, int npoly, int *nper, Rboolean winding, const pGEcontext gc, pDevDesc dd);
  void (*raster)(unsigned int *raster, int w, int h, double x, double y, double width, double height, double rot, Rboolean interpolate, const pGEcontext gc, pDevDesc dd);
  SEXP (*cap)(pDevDesc dd);
  void (*size)(double *left, double *right, double *bottom, double *top, pDevDesc dd);
  double (*strWidth)(const char *str, const pGEcontext gc, pDevDesc dd);
  void (*text)(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc, pDevDesc dd);
  void (*onExit)(pDevDesc dd);
  SEXP (*getEvent)(SEXP, const char *);
  Rboolean (*newFrameConfirm)(pDevDesc dd);
  Rboolean hasTextUTF8;
  void (*textUTF8)(double x, double y, const char *str, double rot, double hadj, const pGEcontext gc, pDevDesc dd);
  double (*strWidthUTF8)(const char *str, const pGEcontext gc, pDevDesc dd);
  Rboolean wantSymbolUTF8;
  Rboolean useRotatedTextInContour;
  SEXP eventEnv;
  void (*eventHelper)(pDevDesc dd, int code);
  int (*holdflush)(pDevDesc dd, int level);
  int haveTransparency;
  int haveTransparentBg;
  int haveRaster;
  int haveCapture, haveLocator;
  SEXP (*setPattern)(SEXP pattern, pDevDesc dd);
  void (*releasePattern)(SEXP ref, pDevDesc dd);
  SEXP (*setClipPath)(SEXP path, SEXP ref, pDevDesc dd);
  void (*releaseClipPath)(SEXP ref, pDevDesc dd);
  SEXP (*setMask)(SEXP path, SEXP ref, pDevDesc dd);
  void (*releaseMask)(SEXP ref, pDevDesc dd);
  int deviceVersion;
  Rboolean deviceClip;
  char reserved[64];
};
#endif //RWRAPPER_RVERSIONHELPER_H
