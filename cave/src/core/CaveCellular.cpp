#include "CaveCellular.h"

#include <vector>

#include "Range.h"
#include "RandSimple.h"
#include "RogueCave.hpp"
#include "SimplexNoise.h"

namespace Cave {

TileMap CaveCellular::generate(int width, int height,
                               const GenerationParams &params) {
  const CellularParams &cp = params.cellular;
  RNG::RandSimple simple(params.seed);
  TileMap map(static_cast<size_t>(height),
              std::vector<int>(static_cast<size_t>(width), WALL));

  // Fill with random or simplex noise.
  const double W = width - 1 + cp.mAmp;
  const double H = height - 1 + cp.mAmp;
  // Seed-derived z-slice so Simplex output varies per seed (matches
  // CaveHeightmap pattern)
  const double zOffset = (params.seed % 10000) / 100.0;
  for (int cy = 0; cy < height; ++cy) {
    for (int cx = 0; cx < width; ++cx) {
      double x = cx / W * cp.mFreq;
      double y = cy / H * cp.mFreq;
      double n1 = cp.mPerlin ? Algo::getSNoise3(x, y, zOffset, cp.mOctaves)
                             : simple.getFloat() - cp.mWallChance;
      map[cy][cx] = (n1 < 0) ? WALL : FLOOR;
    }
  }

  if (cp.mGenerations.empty()) {
    return map;
  }

  // Run the cellular automata.
  PCG::RogueCave cave(width, height);
  std::vector<std::vector<int>> &gridIn = cave.getGrid();
  for (int cy = 0; cy < height; ++cy) {
    for (int cx = 0; cx < width; ++cx) {
      gridIn[cy][cx] = map[cy][cx] == WALL ? PCG::RogueCave::TILE_WALL
                                           : PCG::RogueCave::TILE_FLOOR;
    }
  }
  for (const auto &gen : cp.mGenerations) {
    Util::IntRange b3(gen.b3_min, gen.b3_max);
    Util::IntRange b5(gen.b5_min, gen.b5_max);
    Util::IntRange s3(gen.s3_min, gen.s3_max);
    Util::IntRange s5(gen.s5_min, gen.s5_max);
    cave.addGeneration(b3, b5, s3, s5, gen.reps);
  }
  std::vector<std::vector<int>> &gridOut = cave.generate();
  for (int cy = 0; cy < height; ++cy) {
    for (int cx = 0; cx < width; ++cx) {
      map[cy][cx] = (gridOut[cy][cx] == PCG::RogueCave::TILE_WALL) ? WALL : FLOOR;
    }
  }
  return map;
}

} // namespace Cave
