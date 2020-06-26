#ifndef RWRAPPER_SNAPSHOTTYPE_H
#define RWRAPPER_SNAPSHOTTYPE_H

namespace graphics {

enum class SnapshotType {
  SKETCH,
  NORMAL,
};

inline const char* toString(SnapshotType type) {
  if (type == SnapshotType::SKETCH) {
    return "sketch";
  } else {
    return "normal";
  }
}

}  // graphics

#endif //RWRAPPER_SNAPSHOTTYPE_H
