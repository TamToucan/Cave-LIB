#ifndef VORONOI_NOISE_H
#define VORONOI_NOISE_H

#include "GenerationParams.h"
#include "TileTypes.h"
#include <cstdint>

namespace Cave {

class VoronoiNoise {
public:
  static TileMap generate(int W, int H, const GenerationParams &params);

private:
  static float worleyMetric(float px, float py, uint32_t seed);
  static void smoothGrid(TileMap &g);
};

} // namespace Cave

#endif // VORONOI_NOISE_H
