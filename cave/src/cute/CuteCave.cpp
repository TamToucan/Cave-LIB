#include "CuteCave.hpp"

#include <cute.h>

#include "Cave.h"
#include "CaveInfo.h"
#include "Debug.h"
#include "GenerationParams.h"
#include "TileTypes.h"

///////////////////////////////////////////////////////////////////////

namespace CuteCave {

CuteCave::CuteCave() {}

CuteCave &CuteCave::setCaveSize(int width, int height) {
  m_info.mCaveWidth = width;
  m_info.mCaveHeight = height;
  return *this;
}

CuteCave &CuteCave::setBorderCellSize(int width, int height) {
  m_info.mBorderWidth = width;
  m_info.mBorderHeight = height;
  return *this;
}

CuteCave &CuteCave::setCellSize(int width, int height) {
  m_info.mCellWidth = width;
  m_info.mCellHeight = height;
  return *this;
}

CuteCave &CuteCave::setCaveType(Cave::CaveType type) {
  m_gen_params.mCaveType = type;
  return *this;
}

CuteCave &CuteCave::setStartCell(int x, int y) {
  m_info.mStartCellX = x;
  m_info.mStartCellY = y;
  return *this;
}

CuteCave &CuteCave::setOctaves(int octaves) {
  m_gen_params.cellular.mOctaves = octaves;
  return *this;
}

CuteCave &CuteCave::setWallChance(float wallChance) {
  m_gen_params.cellular.mWallChance = wallChance;
  return *this;
}

CuteCave &CuteCave::setPerlin(bool usePerlin) {
  m_gen_params.cellular.mPerlin = usePerlin;
  return *this;
}

CuteCave &CuteCave::setFreq(float freq) {
  m_gen_params.cellular.mFreq = freq;
  return *this;
}

CuteCave &CuteCave::setAmp(float amp) {
  m_gen_params.cellular.mAmp = amp;
  return *this;
}

CuteCave &CuteCave::setSmoothing(bool doSmoothing) {
  m_info.mSmoothing = doSmoothing;
  return *this;
}

CuteCave &CuteCave::setSmoothCorners(bool doSmoothCorners) {
  m_info.mSmoothCorners = doSmoothCorners;
  return *this;
}

CuteCave &CuteCave::setSmoothPoints(bool doSmoothPoints) {
  m_info.mSmoothPoints = doSmoothPoints;
  return *this;
}

CuteCave &CuteCave::setRemoveDiagonals(bool doRemoveDiagonals) {
  m_info.mRemoveDiagonals = doRemoveDiagonals;
  return *this;
}

CuteCave &CuteCave::setGenerations(std::vector<Cave::GenerationStep> gens) {
  m_gen_params.cellular.mGenerations = gens;
  return *this;
}

CuteCave &CuteCave::setVoronoiParams(float noiseScale, float warpStrength,
                                     float voronoiWeight, float threshold,
                                     bool invertVoronoi) {
  m_gen_params.voronoi.mNoiseScale = noiseScale;
  m_gen_params.voronoi.mWarpStrength = warpStrength;
  m_gen_params.voronoi.mVoronoiWeight = voronoiWeight;
  m_gen_params.voronoi.mThreshold = threshold;
  m_gen_params.voronoi.mInvertVoronoi = invertVoronoi;
  return *this;
}

CuteCave &CuteCave::setHeightmapParams(int octaves, float freq,
                                       float waterLevelMin,
                                       float waterLevelMax) {
  m_gen_params.heightmap.mOctaves = octaves;
  m_gen_params.heightmap.mFreq = freq;
  m_gen_params.heightmap.mWaterLevelMin = waterLevelMin;
  m_gen_params.heightmap.mWaterLevelMax = waterLevelMax;
  return *this;
}

CuteCave &CuteCave::setDLAParams(int particleCount, int openAreaCount,
                                 int openAreaRadius) {
  m_gen_params.dla.mParticleCount = particleCount;
  m_gen_params.dla.mOpenAreaCount = openAreaCount;
  m_gen_params.dla.mOpenAreaRadius = openAreaRadius;
  return *this;
}

CuteCave &
CuteCave::setTunnelParams(int minLengthForOrganic, float wiggleAmplitude,
                          float wiggleFrequency, float widthPulseAmplitude,
                          float widthPulseFrequency, int extraConnections,
                          float extraConnectionChance) {
  m_gen_params.tunnel.mMinLengthForOrganic = minLengthForOrganic;
  m_gen_params.tunnel.mWiggleAmplitude = wiggleAmplitude;
  m_gen_params.tunnel.mWiggleFrequency = wiggleFrequency;
  m_gen_params.tunnel.mWidthPulseAmplitude = widthPulseAmplitude;
  m_gen_params.tunnel.mWidthPulseFrequency = widthPulseFrequency;
  m_gen_params.tunnel.mExtraConnections = extraConnections;
  m_gen_params.tunnel.mExtraConnectionChance = extraConnectionChance;
  return *this;
}

CuteCave &CuteCave::setBspTectonicParams(int depth, int minRoomSize,
                                         int roomPadding) {
  m_gen_params.bspTectonic.mBspDepth = depth;
  m_gen_params.bspTectonic.mMinRoomSize = minRoomSize;
  m_gen_params.bspTectonic.mRoomPadding = roomPadding;
  return *this;
}

CuteCave &
CuteCave::setBspTectonicCaParams(int octaves, bool usePerlin, float wallChance,
                                 float freq, float amp,
                                 std::vector<Cave::GenerationStep> gens) {
  m_gen_params.bspTectonic.mCaParams.mOctaves = octaves;
  m_gen_params.bspTectonic.mCaParams.mPerlin = usePerlin;
  m_gen_params.bspTectonic.mCaParams.mWallChance = wallChance;
  m_gen_params.bspTectonic.mCaParams.mFreq = freq;
  m_gen_params.bspTectonic.mCaParams.mAmp = amp;
  m_gen_params.bspTectonic.mCaParams.mGenerations = gens;
  return *this;
}

///////////////////////////////////////////////////////////////////////

CuteCave::TileAtlas CuteCave::loadTileAtlas(const char *virtual_path,
                                            int tile_size) {
  TileAtlas atlas = {};

  // 1. Load raw image dimensions
  int w, h;
  void *data = NULL;
  size_t sz = 0;

  data = Cute::fs_read_entire_file_to_memory(virtual_path, &sz);
  CF_ASSERT(data && "Failed to read tile image. Ensure 'tiles_64x64.png' is in "
                    "the assets folder.");

  // Just get width/height, don't decode pixels yet
  Cute::image_load_png_wh(data, (int)sz, &w, &h);
  cf_free(data);

  atlas.width_in_tiles = w / tile_size; // Should be 512 / 64 = 8
  int height_in_tiles = h / tile_size;  // Should be 512 / 64 = 8
  int sub_image_count = atlas.width_in_tiles * height_in_tiles;

  LOG_ASSERT((atlas.width_in_tiles == 8) && (height_in_tiles == 8),
             "Atlas tile width should be 8x8: not: "
                 << atlas.width_in_tiles << " x " << height_in_tiles);

  // 2. Define UVs for 8x8 grid
  static uint64_t global_image_id_counter = 0;
  atlas.base_id = global_image_id_counter;

  std::vector<CF_AtlasSubImage> sub_images(sub_image_count);

  for (int i = 0; i < sub_image_count; ++i) {
    CF_AtlasSubImage &sub = sub_images[i];
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
    int atlas_index = Cave::Cave::getAtlasIndex(i);
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

bool CuteCave::hasFloorSpace(const Cave::TileMap &tileMap) const {
  Cave::Cave cave(m_info, m_gen_params);
  return cave.hasFloorSpace(tileMap);
}

std::string CuteCave::getParamsString() const {
  Cave::Cave cave(m_info, m_gen_params);
  return cave.getParamsString();
}

} // namespace CuteCave
