#ifndef VORONOI_NOISE_H
#define VORONOI_NOISE_H

#include "CaveInfo.h"
#include "TileTypes.h"
#include <cstdint>
#include <vector>

#include "GenerationParams.h"

namespace Cave {

class VoronoiNoise {
public:
  static TileMap generate(int W, int H, const GenerationParams &params);

private:
  static std::vector<Vector2i> generateSeeds(int W, int H, int cellsX,
                                             int cellsY, uint32_t seed);
  static float voronoiMetric(int x, int y, const std::vector<Vector2i> &seeds);

  static void smoothGrid(TileMap &g);
};

} // namespace Cave

#endif // VORONOI_NOISE_H
