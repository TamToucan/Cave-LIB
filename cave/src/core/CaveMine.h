#ifndef CAVE_MINE_H
#define CAVE_MINE_H

#include "GenerationParams.h"
#include "TileTypes.h"

namespace Cave {

/**
 * @class CaveMine
 * @brief Room-and-pillar mine generator.
 * @details SPECIFICATION: specs/features/cave_gen_mine.md. A main of parallel
 * entries and cross-cuts runs the long axis through the middle; walled panels
 * open off both sides through barrier openings. Each panel is a pillar grid,
 * a robbed grid (pillars removed, some left as crushed stubs), one longwall
 * hall, or unmined rock. Connected by construction: every mined panel opens
 * into the main. Halls and 2-wide openings make each mined panel and the
 * main a separate room for the game's room finder. Integer geometry; one RNG
 * stream in a fixed order.
 */
class CaveMine {
public:
  /**
   * @brief Generate a mine TileMap of size width x height (WALL / FLOOR).
   * @param params Reads MineParams (params.mine) and params.seed.
   */
  static TileMap generate(int width, int height,
                          const GenerationParams &params);
};

} // namespace Cave

#endif
