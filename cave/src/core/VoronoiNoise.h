#ifndef VORONOI_NOISE_H
#define VORONOI_NOISE_H

/**
 * @file VoronoiNoise.h
 * @brief Voronoi-noise cave generator backend.
 * @details Samples a domain-warped Worley/Voronoi distance field and
 * thresholds it to produce a chambered cave layout. Parameters come from
 * GenerationParams::voronoi.
 */

#include "GenerationParams.h"
#include "TileTypes.h"
#include <cstdint>

namespace Cave {

/**
 * @class VoronoiNoise
 * @brief Worley/Voronoi-based cave generator.
 */
class VoronoiNoise {
public:
  /**
   * @brief Generate a Voronoi TileMap of size W x H.
   * @param params Pulls VoronoiParams (noise scale, warp strength, weight,
   *               threshold, invert flag) and shared seed.
   */
  static TileMap generate(int W, int H, const GenerationParams &params);

private:
  /// F1 Worley metric — distance to nearest jittered lattice feature point.
  static float worleyMetric(float px, float py, uint32_t seed);
  /// Light post-pass to remove single-cell speckle in the threshold result.
  static void smoothGrid(TileMap &g);
};

} // namespace Cave

#endif // VORONOI_NOISE_H
