#include "CuteCave.hpp"

#include <cute.h>

#include "Cave.h"
#include "CaveInfo.h"
#include "Debug.h"
#include "GenerationParams.h"
#include "TileTypes.h"


namespace {

int getAtlasIndex(Cave::TileName tile) {
  // Helper lambda to calculate index: Row * 8 + Col
  // Assuming 0,0 is Top-Left
  auto Idx = [](int col, int row) { return (row * 8) + col; };

  switch (tile) {
    case Cave::FLOOR:
      return Idx(0, 7);
    case Cave::WALL:
      return Idx(1, 7);

    case Cave::T45a:
      return Idx(2, 6);
    case Cave::T45b:
      return Idx(3, 6);
    case Cave::T45c:
      return Idx(0, 6);
    case Cave::T45d:
      return Idx(1, 6);

    case Cave::V60a1:
      return Idx(2, 3);
    case Cave::V60a2:
      return Idx(2, 4);
    case Cave::V60b1:
      return Idx(1, 3);
    case Cave::V60b2:
      return Idx(1, 4);
    case Cave::V60c1:
      return Idx(3, 4);
    case Cave::V60c2:
      return Idx(3, 3);
    case Cave::V60d1:
      return Idx(0, 4);
    case Cave::V60d2:
      return Idx(0, 3);

    case Cave::H30a1:
      return Idx(2, 5);
    case Cave::H30a2:
      return Idx(3, 5);
    case Cave::H30b1:
      return Idx(7, 5);
    case Cave::H30b2:
      return Idx(6, 5);
    case Cave::H30c1:
      return Idx(1, 5);
    case Cave::H30c2:
      return Idx(0, 5);
    case Cave::H30d1:
      return Idx(4, 5);
    case Cave::H30d2:
      return Idx(5, 5);

    case Cave::END_N:
      return Idx(4, 6);
    case Cave::END_S:
      return Idx(6, 6);
    case Cave::END_E:
      return Idx(5, 6);
    case Cave::END_W:
      return Idx(7, 6);

    case Cave::DEND_N:
      return Idx(4, 7);
    case Cave::DEND_S:
      return Idx(6, 7);
    case Cave::DEND_E:
      return Idx(5, 7);
    case Cave::DEND_W:
      return Idx(7, 7);

    case Cave::SINGLE:
      return Idx(3, 7);

    default:
      return Idx(0, 7);  // Default to FLOOR
  }
}

}  // namespace

///////////////////////////////////////////////////////////////////////

namespace CuteCave {

CuteCave::CuteCave() {
}

CuteCave& CuteCave::setCaveSize(int width, int height) {
  m_info.mCaveWidth = width;
  m_info.mCaveHeight = height;
  return *this;
}

CuteCave& CuteCave::setBorderCellSize(int width, int height) {
  m_info.mBorderWidth = width;
  m_info.mBorderHeight = height;
  return *this;
}

CuteCave& CuteCave::setCellSize(int width, int height) {
  m_info.mCellWidth = width;
  m_info.mCellHeight = height;
  return *this;
}

CuteCave& CuteCave::setStartCell(int x, int y) {
  m_info.mStartCellX = x;
  m_info.mStartCellY = y;
  return *this;
}

CuteCave& CuteCave::setOctaves(int octaves) {
  m_gen_params.mOctaves = octaves;
  return *this;
}

CuteCave& CuteCave::setWallChance(float wallChance) {
  m_gen_params.mWallChance = wallChance;
  return *this;
}

CuteCave& CuteCave::setPerlin(bool usePerlin) {
  m_gen_params.mPerlin = usePerlin;
  return *this;
}

CuteCave& CuteCave::setFreq(float freq) {
  m_gen_params.mFreq = freq;
  return *this;
}

CuteCave& CuteCave::setAmp(float amp) {
  m_gen_params.mAmp = amp;
  return *this;
}

CuteCave& CuteCave::setGenerations(std::vector<Cave::GenerationStep> gens) {
  m_gen_params.mGenerations = gens;
  return *this;
}

///////////////////////////////////////////////////////////////////////

CuteCave::TileAtlas CuteCave::loadTileAtlas(const char* virtual_path,
                                            int tile_size) {
  TileAtlas atlas = {};

  // 1. Load raw image dimensions
  int w, h;
  void* data = NULL;
  size_t sz = 0;

  data = Cute::fs_read_entire_file_to_memory(virtual_path, &sz);
  CF_ASSERT(data &&
            "Failed to read tile image. Ensure 'tiles_64x64.png' is in "
            "the assets folder.");

  // Just get width/height, don't decode pixels yet
  Cute::image_load_png_wh(data, (int)sz, &w, &h);
  cf_free(data);

  atlas.width_in_tiles = w / tile_size;  // Should be 512 / 64 = 8
  int height_in_tiles = h / tile_size;   // Should be 512 / 64 = 8
  int sub_image_count = atlas.width_in_tiles * height_in_tiles;

  LOG_ASSERT((atlas.width_in_tiles == 8) && (height_in_tiles == 8),
             "Atlas tile width should be 8x8: not: "
                 << atlas.width_in_tiles << " x " << height_in_tiles);

  // 2. Define UVs for 8x8 grid
  static uint64_t global_image_id_counter = 0;
  atlas.base_id = global_image_id_counter;

  std::vector<CF_AtlasSubImage> sub_images(sub_image_count);

  for (int i = 0; i < sub_image_count; ++i) {
    CF_AtlasSubImage& sub = sub_images[i];
    int x = i % atlas.width_in_tiles;
    int y = i / atlas.width_in_tiles;

    sub.image_id = global_image_id_counter++;
    sub.w = tile_size;
    sub.h = tile_size;

    // inset to sample middle of pixel to stop bleeding
    const float tex_w = (float)w;
    const float tex_h = (float)h;
    const float inset = 0.5f;

    // UV Coordinates (0,0 is top-left)
    sub.minx = (float)(x * tile_size + inset) / tex_w;
    sub.miny = (float)((y + 1) * tile_size - inset) / tex_h;
    sub.maxx = (float)((x + 1) * tile_size - inset) / tex_w;
    sub.maxy = (float)(y * tile_size + inset) / tex_h;
  }

  // 3. Register atlas
  atlas.texture = cf_register_premade_atlas(virtual_path, sub_image_count,
                                            sub_images.data());

  // 4. Pre-create Sprites
  for (int i = 0; i < Cave::TileName::TILE_COUNT; ++i) {
    int atlas_index = getAtlasIndex((Cave::TileName)i);
    atlas.tile_sprites[i] =
        Cute::make_premade_sprite(atlas.base_id + atlas_index);
  }

  return atlas;
}

///////////////////////////////////////////////////////////////////////

const Cave::TileMap CuteCave::make_cave(int seed) {
  m_gen_params.seed = seed;

  Cave::Cave cave(m_info, m_gen_params);
  return cave.generate();
}

}  // namespace CuteCave
