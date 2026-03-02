#ifndef CAVE_HEIGHTMAP_H
#define CAVE_HEIGHTMAP_H

#include "GenerationParams.h"
#include "TileTypes.h"
#include <vector>


namespace Cave {

class CaveHeightmap {
public:
  static TileMap generate(int width, int height,
                          const GenerationParams &params);
};

} // namespace Cave

#endif // CAVE_HEIGHTMAP_H
