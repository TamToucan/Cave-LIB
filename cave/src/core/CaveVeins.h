#ifndef CAVE_VEINS_H
#define CAVE_VEINS_H

#include "GenerationParams.h"
#include "TileTypes.h"

namespace Cave {

/**
 * @class CaveVeins
 * @brief Vein-network generator (space colonization, Runions et al. 2005).
 * @details SPECIFICATION: specs/features/cave_gen_veins.md. Attractors on a
 * jittered grid pull vein tips from one or more roots; each step a node
 * grows one segment toward the attractors nearest to it, and attractors
 * within killDist are eaten. Width follows the pipe rule (sqrt of the tips
 * below). Tips may end in a chamber; cross-links make loops. Floats use only
 * + - * / and sqrt; sums run in a fixed order.
 */
class CaveVeins {
public:
  /**
   * @brief Generate a vein TileMap of size width x height (WALL / FLOOR).
   * @param params Reads VeinsParams (params.veins) and params.seed.
   */
  static TileMap generate(int width, int height,
                          const GenerationParams &params);
};

} // namespace Cave

#endif
