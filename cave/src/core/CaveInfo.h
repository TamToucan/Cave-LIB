#ifndef CAVE_INFO_H
#define CAVE_INFO_H

namespace Cave {

struct Vector2i {
  int x = 0;
  int y = 0;

  bool operator==(const Vector2i& other) const {
    return x == other.x && y == other.y;
  }

  bool operator<(const Vector2i& other) const {
    if (x < other.x)
      return true;
    if (x > other.x)
      return false;
    return y < other.y;
  }
};

struct CaveInfo {
  bool mSmoothing = true;
  bool mSmoothCorners = true;
  bool mSmoothPoints = true;
  int mCaveWidth = 2;
  int mCaveHeight = 2;
  int mBorderWidth = 1;
  int mBorderHeight = 1;
  int mCellWidth = 1;
  int mCellHeight = 1;
  int mStartCellX = 0;
  int mStartCellY = 0;
  int mLayer = 0;
};

}  // namespace Cave

#endif
