#ifndef CAVE_CELLULAR_H
#define CAVE_CELLULAR_H

#include "GenerationParams.h"
#include "TileTypes.h"

namespace Cave {

/**
 * @class CaveCellular
 * @brief Classic cellular-automata cave generator.
 * @details SPECIFICATION: specs/features/cave_gen_zones.md §2 (moved out of
 * Cave so CaveZones can run it in a slot). Seeds the grid with random noise
 * (RandSimple, mWallChance) or simplex noise (mPerlin), then runs the
 * CellularParams generations through PCG::RogueCave. Same output as the old
 * Cave::initialise + Cave::runCellularAutomata.
 */
class CaveCellular {
public:
  /**
   * @brief Generate a cellular TileMap of size width x height (WALL / FLOOR).
   * @param params Reads CellularParams (params.cellular) and params.seed.
   */
  static TileMap generate(int width, int height,
                          const GenerationParams &params);
};

} // namespace Cave

#endif
