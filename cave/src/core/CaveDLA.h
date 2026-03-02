#ifndef CAVE_DLA_H
#define CAVE_DLA_H

#include "GenerationParams.h"
#include "TileTypes.h"

namespace Cave {

/**
 * @class CaveDLA
 * @brief Diffusion-Limited Aggregation (DLA) cave generator.
 * @details SPECIFICATION: Generates coral-like/crystalline caves by starting
 * with a central floor tile and simulating random-walking particles from the
 * map edge until they touch a floor tile and freeze. Also carves out a
 * configurable number of open spaces.
 */
class CaveDLA {
public:
  static TileMap generate(int width, int height,
                          const GenerationParams &params);
};

} // namespace Cave

#endif
