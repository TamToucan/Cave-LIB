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
  // End-caps
  END_N,
  END_S,
  END_E,
  END_W,
  //////////////////////////////////////
  // These tiles are all the "floor" tiles
  // The completely empty tile
  FLOOR,
  // Round both corners of dead-ends
  DEND_N,
  DEND_S,
  DEND_E,
  DEND_W,
  // round one corner right-angle
  CORNR_A,
  CORNR_B,
  CORNR_C,
  CORNR_D,
  // Mark the range
  FLOOR_START = FLOOR,
  FLOOR_END = CORNR_D,
  //////////////////////////////////////

  // Cut 2 corners off the 45 degree tile
  T45a2CT,
  T45b2CT,
  T45c2CT,
  T45d2CT,

  // Cut 1 (of 2)corners off the 45 degree tile
  T45abCT,
  T45adCT,
  T45baCT,
  T45bcCT,
  T45cbCT,
  T45cdCT,
  T45daCT,
  T45dcCT,

  // Cut the sharp end of the 60 degree tiles
  V60aCT,
  V60bCT,
  V60cCT,
  V60dCT,

  // Cut the sharp end of the 30 degree tiles
  H30aCT,
  H30bCT,
  H30cCT,
  H30dCT,

  // Generic wall, input to the smoother.
  WALL,

  // Special values for smoother internal grids.
  SOLID,
  TILE_COUNT,

  // NOTE: No sprite created past TILE_COUNT
  IGNORE
};

}  // namespace Cave

#endif
