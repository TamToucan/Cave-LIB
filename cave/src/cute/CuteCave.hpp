#ifndef CUTE_CAVE_HPP
#define CUTE_CAVE_HPP

#include <cute.h>

#include "CaveInfo.h"
#include "GenerationParams.h"
#include "TileTypes.h"

namespace CuteCave {

class CuteCave {
 public:
  struct TileAtlas {
    CF_Texture texture;
    uint64_t base_id;
    int width_in_tiles;  // Should be 512 / 64 = 8 for 512x512 (64x64 tiles)
    CF_Sprite tile_sprites[Cave::TileName::TILE_COUNT];
  };

 public:
  CuteCave();
  CuteCave& setCaveSize(int width, int height);
  CuteCave& setBorderCellSize(int width, int height);
  CuteCave& setCellSize(int width, int height);
  CuteCave& setStartCell(int x, int y);
  CuteCave& setOctaves(int octaves);
  CuteCave& setPerlin(bool usePerlin);
  CuteCave& setWallChance(float wallChance);
  CuteCave& setFreq(float freq);
  CuteCave& setAmp(float amp);
  CuteCave& setSmoothing(bool doSmoothing);
  CuteCave& setSmoothCorners(bool doSmoothCorners);
  CuteCave& setSmoothPoints(bool doSmoothPoints);
  CuteCave& setGenerations(std::vector<Cave::GenerationStep> gens);

  TileAtlas loadTileAtlas(const char* virtual_path, int tile_size);

  const Cave::TileMap make_cave(int seed);

 private:
  Cave::CaveInfo m_info;
  Cave::GenerationParams m_gen_params;
};

}  // namespace CuteCave

#endif  // CUTE_CAVE_HPP
