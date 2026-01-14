#ifndef TILE_TYPES_H
#define TILE_TYPES_H
#include <vector>

namespace Cave {
using TileMap = std::vector<std::vector<int>>;

// TileName is used to identify the type of tile to be placed in the map.
// This is used by the core library and the Godot wrapper will map these
// to actual tile atlas coordinates.
enum TileName {
  T45a,
  T45b,
  T45c,
  T45d,
  V60a1,
  V60a2,
  V60b1,
  V60b2,
  V60c1,
  V60c2,
  V60d1,
  V60d2,
  H30a1,
  H30a2,
  H30b1,
  H30b2,
  H30c1,
  H30c2,
  H30d1,
  H30d2,
  SINGLE,
  END_N,
  END_S,
  END_E,
  END_W,
  DEND_N,
  DEND_S,
  DEND_E,
  DEND_W,

  // Generic wall, input to the smoother.
  WALL,

  // Special values for smoother internal grids.
  SOLID,
  FLOOR,
  TILE_COUNT,

  SMOOTHED,  // NOTE: No sprite created for these
  IGNORE
};

}  // namespace Cave

#endif
