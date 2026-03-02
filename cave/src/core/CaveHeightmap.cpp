#include "CaveHeightmap.h"
#include "RandSimple.h"
#include "SimplexNoise.h"
#include "TileTypes.h"

namespace Cave {

TileMap CaveHeightmap::generate(int width, int height,
                                const GenerationParams &params) {
  TileMap outMap(height, std::vector<int>(width, WALL));

  RNG::RandSimple simple(params.seed);

  float rangeLevel =
      params.heightmap.mWaterLevelMax - params.heightmap.mWaterLevelMin;
  float waterLevel =
      params.heightmap.mWaterLevelMin + (simple.getFloat() * rangeLevel);

  // Z-offset for organic variation per seed
  double zOffset = simple.getFloat() * 10000.0;

  const double W = width - 1;
  const double H = height - 1;

  for (int cy = 0; cy < height; ++cy) {
    for (int cx = 0; cx < width; ++cx) {
      double x = (cx / W) * params.heightmap.mFreq;
      double y = (cy / H) * params.heightmap.mFreq;

      // Using Simplex Noise based on the specified octaves
      double n = Algo::getSNoise3(x, y, zOffset, params.heightmap.mOctaves);

      if (n < waterLevel) {
        outMap[cy][cx] = FLOOR;
      } else {
        outMap[cy][cx] = WALL;
      }
    }
  }

  return outMap;
}

} // namespace Cave
