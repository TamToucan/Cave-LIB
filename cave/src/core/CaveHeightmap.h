#ifndef CAVE_HEIGHTMAP_H
#define CAVE_HEIGHTMAP_H

#include "GenerationParams.h"
#include "TileTypes.h"
#include <vector>


namespace Cave {

/**
 * @class CaveHeightmap
 * @brief Fractal Heightmap cave generator.
 * @details Generates an organic cave layout by sampling fractal Simplex noise
 * and applying a "water level" threshold: cells below the threshold become
 * FLOOR, cells above become WALL. Configured via HeightmapParams in
 * GenerationParams.
 */
class CaveHeightmap {
public:
  /**
   * @brief Generate a Heightmap TileMap of size width x height.
   * @param params Pulls HeightmapParams (octaves, frequency, water-level range).
   */
  static TileMap generate(int width, int height,
                          const GenerationParams &params);
};

} // namespace Cave

#endif // CAVE_HEIGHTMAP_H
