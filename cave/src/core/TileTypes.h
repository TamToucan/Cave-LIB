#ifndef TILE_TYPES_H
#define TILE_TYPES_H

/**
 * @file TileTypes.h
 * @brief Tile-id enum and TileMap alias used across all Cave generators.
 * @details TileMap is a 2D vector<int> indexed [y][x] storing TileName values.
 * Wrappers (e.g. Godot's GDCave, the Cute CuteCave) translate these ids to
 * concrete sprite atlas indices. Values past TILE_COUNT (IGNORE) are reserved
 * sentinels and never rendered.
 */

#include <vector>

namespace Cave {
/// 2D tile grid indexed as [y][x], each cell holding a TileName value.
using TileMap = std::vector<std::vector<int>>;

// TileName is used to identify the type of tile to be placed in the map.
// This is used by the core library and the Godot wrapper will map these
// to actual tile atlas coordinates.
//
// The naming convention is as follows:
// - T45a, T45b, T45c, T45d are 45 degree angles
// - V60a, V60b, V60c, V60d are 60 degree angles
// - H30a, H30b, H30c, H30d are 30 degree angles
//
// - Letters a-d refer to the four corners of the tile (a=TL, b=TR, c=BR, d=BL)
//
// - The option 1 or 2 at the end, e.g. H30a1, refers to the "thich" end of
//   a tile pair
//
// For exampleto make a  60 degree slope with the right angle at the
// bottom left requires 2 tiles; V60d1 and (above it) H60d2
//
//         a
//   V60d2 |\     "thin" end
//         | \
//   V60d1 |  \
//         |__\  "thicj" end
//         d   c
//
// -2CT means both corners are cut off. This is ontl T45 tiles (since
//  a single H30 and V60  tile only has 1 corner)

// - 1CT means the thick end corner is off. This would be corner 'c' in
//   the above diagram. The ilName would be V60d1CT (V60d1CT with cut corner))

// - CT wihotu 1 or 2 means the thin end corner is cut off. This would be
//   corner 'a' in the above diagram. The TilName would be V60dCT
//
// - CORNER refers to a small "corner tile" in the conner give by
//   A,B,C,D (same a,b,c,d)
//
// - All tiles are symetrical and can be rotated and flipped
// - END_N, END_S, END_E, END_W are symetrical e.g. END N is
//   the same even when flipped horontally. When flipped
//   vertically it equals END_S.
//

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

  // Cut the thick end of the 60 degree tiles
  V60a1CT,
  V60b1CT,
  V60c1CT,
  V60d1CT,

  // Cut the thick end of the 30 degree tiles
  H30a1CT,
  H30b1CT,
  H30c1CT,
  H30d1CT,

  // Ladder tiles for STAIR triggers. Not in the FLOOR range: placed by a
  // post-generation tile-graphic swap (Game::dispatchPlan_), so the wall grid
  // and DistanceMap are already baked and unaffected. Passable (no collider).
  LADDER_DN,
  LADDER_UP,
  STAIRS_DN,
  STAIRS_UP,

  CAMPFIRE,

  // CHESTs placed as above via client, they open with with lid moving N/S/E/W
  CHEST_CLOSE_N,
  CHEST_CLOSE_S,
  CHEST_CLOSE_E,
  CHEST_CLOSE_W,
  CHEST_OPEN_N,
  CHEST_OPEN_S,
  CHEST_OPEN_E,
  CHEST_OPEN_W,

  // Generic wall, input to the smoother.
  WALL,

  // Special values for smoother internal grids.
  SOLID,
  TILE_COUNT,

  // NOTE: No sprite created past TILE_COUNT
  IGNORE
};

/// First tile id reserved for client-defined template tiles. Ids >= this are
/// never produced by the core generator; they are owned entirely by the client
/// (see CuteLott specs/features/client_template_tiles.md). Power-of-2, well
/// past IGNORE so built-in TileName values stay frozen.
constexpr int CLIENT_TILE_BASE = 1024;

} // namespace Cave

#endif
