#ifndef CAVE_INFO_H
#define CAVE_INFO_H

/**
 * @file CaveInfo.h
 * @brief Shared sizing/Vector types for Cave generators.
 * @details Defines Vector2i (integer 2D point with ordering for use in
 * std::map) and CaveInfo (cave grid size and smoothing toggles). CaveInfo is
 * passed alongside GenerationParams when constructing a Cave.
 */

namespace Cave {

/// Integer 2D point. Provides equality + lexicographic ordering for set/map use.
struct Vector2i {
  int x = 0;
  int y = 0;

  bool operator==(const Vector2i &other) const {
    return x == other.x && y == other.y;
  }

  bool operator<(const Vector2i &other) const {
    if (x < other.x)
      return true;
    if (x > other.x)
      return false;
    return y < other.y;
  }
};

/**
 * @struct CaveInfo
 * @brief Grid size and post-processing smoothing toggles for a Cave.
 * @details mCaveWidth/mCaveHeight set the working grid size; the mSmooth*
 * flags toggle stages in CaveSmoother. Border/Cell/StartCell fields are
 * retained only for the Godot GDCave wrapper.
 */
struct CaveInfo {
  bool mSmoothing = true;     // NOTE: Piority over mRemoveDiagonals
  bool mSmoothCorners = true; // NOTE: not used if mSmoothing is false
  bool mSmoothPoints = true;  // NOTE: not used if mSmoothing is false
  int mCaveWidth = 2;
  int mCaveHeight = 2;
  int mBorderWidth = 1;  // Only used for Godot GDCave
  int mBorderHeight = 1; // Only used for Godot GDCave
  int mCellWidth = 1;    // Only used for Godot GDCave
  int mCellHeight = 1;   // Only used for Godot GDCave
  int mStartCellX = 0;   // Not used yet
  int mStartCellY = 0;   // Not used yet
  int mLayer = 0;
};

} // namespace Cave

#endif
