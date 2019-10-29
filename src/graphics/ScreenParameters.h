#ifndef RWRAPPER_SCREENPARAMETERS_H
#define RWRAPPER_SCREENPARAMETERS_H

struct Size {
  double width;
  double height;
};

inline Size operator*(Size size, double alpha) {
  return Size { size.width * alpha, size.height * alpha };
}

inline Size operator*(double alpha, Size size) {
  return size * alpha;
}

struct ScreenParameters {
  Size size;
  int resolution;
};

#endif //RWRAPPER_SCREENPARAMETERS_H
