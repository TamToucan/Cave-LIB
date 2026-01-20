#include "GDCave.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "Debug.h"
#include "core/Cave.h"
#include "core/TileTypes.h"

using namespace godot;

void GDCave::_bind_methods() {
  ClassDB::bind_method(D_METHOD("set_cave_size", "caveSize"),
                       &GDCave::setCaveSize);
  ClassDB::bind_method(D_METHOD("set_border_cell_size", "cells"),
                       &GDCave::setBorderCellSize);
  ClassDB::bind_method(D_METHOD("set_cell_size", "cells"),
                       &GDCave::setCellSize);
  ClassDB::bind_method(D_METHOD("set_start_cell", "x", "y"),
                       &GDCave::setStartCell);
  ClassDB::bind_method(D_METHOD("set_floor", "floorTileCoords"),
                       &GDCave::setFloor);
  ClassDB::bind_method(D_METHOD("set_wall", "wallTileCoords"),
                       &GDCave::setWall);
  ClassDB::bind_method(D_METHOD("set_octaves", "octaves"), &GDCave::setOctaves);
  ClassDB::bind_method(D_METHOD("set_perlin", "usePerlin"), &GDCave::setPerlin);
  ClassDB::bind_method(D_METHOD("set_wall_chance", "wallChance"),
                       &GDCave::setWallChance);
  ClassDB::bind_method(D_METHOD("set_freq", "freq"), &GDCave::setFreq);
  ClassDB::bind_method(D_METHOD("set_amp", "amp"), &GDCave::setAmp);
  ClassDB::bind_method(D_METHOD("set_generations", "gens"),
                       &GDCave::setGenerations);
  ClassDB::bind_method(D_METHOD("make_cave", "pTileMap", "layer", "seed"),
                       &GDCave::make_cave);
}

GDCave::GDCave() {
  m_floor_tile = Vector2i(0, 0);
  m_wall_tile = Vector2i(0, 1);
}

GDCave::~GDCave() {}

GDCave* GDCave::setCaveSize(Vector2i caveSize) {
  m_cave_info.mCaveWidth = caveSize.x;
  m_cave_info.mCaveHeight = caveSize.y;
  return this;
}

GDCave* GDCave::setBorderCellSize(Vector2i cells) {
  m_cave_info.mBorderWidth = cells.x;
  m_cave_info.mBorderHeight = cells.y;
  return this;
}

GDCave* GDCave::setCellSize(Vector2i cells) {
  m_cave_info.mCellWidth = cells.x;
  m_cave_info.mCellHeight = cells.y;
  return this;
}

GDCave* GDCave::setStartCell(int x, int y) {
  m_cave_info.mStartCellX = x;
  m_cave_info.mStartCellY = y;
  return this;
}

GDCave* GDCave::setFloor(godot::Vector2i floor) {
  m_floor_tile = floor;
  return this;
}

GDCave* GDCave::setWall(godot::Vector2i wall) {
  m_wall_tile = wall;
  return this;
}

GDCave* GDCave::setOctaves(int octaves) {
  m_gen_params.mOctaves = octaves;
  return this;
}

GDCave* GDCave::setWallChance(float wallChance) {
  m_gen_params.mWallChance = wallChance;
  return this;
}

GDCave* GDCave::setPerlin(bool usePerlin) {
  m_gen_params.mPerlin = usePerlin;
  return this;
}

GDCave* GDCave::setFreq(float freq) {
  m_gen_params.mFreq = freq;
  return this;
}

GDCave* GDCave::setAmp(float amp) {
  m_gen_params.mAmp = amp;
  return this;
}

GDCave* GDCave::setGenerations(const godot::Array& gens) {
  m_gen_params.mGenerations.clear();
  for (int i = 0; i < gens.size(); ++i) {
    Array gen = gens[i];
    if (gen.size() == 9) {
      Cave::GenerationStep step;
      step.b3_min = gen[0];
      step.b3_max = gen[1];
      step.b5_min = gen[2];
      step.b5_max = gen[3];
      step.s3_min = gen[4];
      step.s3_max = gen[5];
      step.s5_min = gen[6];
      step.s5_max = gen[7];
      step.reps = gen[8];
      m_gen_params.mGenerations.push_back(step);
    } else {
      UtilityFunctions::push_warning("Invalid generation step size");
    }
  }
  return this;
}

void GDCave::make_cave(TileMapLayer* pTileMap, int layer, int seed) {
  m_gen_params.seed = seed;

  Cave::Cave cave(m_cave_info, m_gen_params);
  const Cave::TileMap caveMap = cave.generate();
  copy_core_to_tilemap(pTileMap, layer, caveMap);
  LOG_INFO("CAVE DONE " << caveMap[0].size() << "x" << caveMap.size());
  for (int y = 0; y < caveMap.size(); ++y) {
    for (int x = 0; x < caveMap[0].size(); ++x) {
      Cave::TileName tile_name = static_cast<Cave::TileName>(caveMap[y][x]);
      LOG_DEBUG_CONT(Cave::Cave::isEmpty(tile_name) ? " " : "#");
    }
    LOG_DEBUG("");
  }
}

void GDCave::copy_core_to_tilemap(TileMapLayer* pTileMap, int layer,
                                  const Cave::TileMap& caveMap) {
  const int BW = m_cave_info.mBorderWidth;
  const int BH = m_cave_info.mBorderHeight;
  LOG_INFO("COPYING CORE TO TILEMAP: " << caveMap.size() << "x"
                                       << caveMap[0].size() << " border: " << BW
                                       << "x" << BW);

  // Logical Size (1x1 borders, so logical size is Size - 2)
  int logicalCaveWdt = caveMap[0].size() - 2;
  int logicalCaveHgt = caveMap.size() - 2;

  // Calculate Destination Size (BORDER + cells * cellSize)
  int destWdt =
      m_cave_info.mBorderWidth * 2 + (logicalCaveWdt * m_cave_info.mCellWidth);
  int destHgt = m_cave_info.mBorderHeight * 2 +
                (logicalCaveHgt * m_cave_info.mCellHeight);

  // Loop cave and write tile cells
  for (int sy = 0; sy < caveMap.size(); ++sy) {
    //
    // Calc Y region
    //
    int dStartY;
    int dHeight;
    bool isBorderRow = true;
    if (sy == 0) {
      // Top Border Row
      dStartY = 0;
      dHeight = m_cave_info.mBorderHeight;
    } else if (sy == caveMap.size() - 1) {
      // Bottom Border Row
      dStartY = m_cave_info.mBorderHeight +
                (logicalCaveHgt * m_cave_info.mCellHeight);
      dHeight = m_cave_info.mBorderHeight;
    } else {
      // Inner Cell Row
      // Start after top border, offset by previous rows
      dStartY =
          m_cave_info.mBorderHeight + ((sy - 1) * m_cave_info.mCellHeight);
      dHeight = m_cave_info.mCellHeight;
      isBorderRow = false;
    }

    for (int sx = 0; sx < caveMap[0].size(); ++sx) {
      //
      // Calc X region
      //
      int dStartX;
      int dWidth;
      bool isBorderColumn = true;
      if (sx == 0) {
        // Left Border Column
        dStartX = 0;
        dWidth = m_cave_info.mBorderWidth;
      } else if (sx == caveMap[0].size() - 1) {
        // Right Border Column
        dStartX = m_cave_info.mBorderWidth +
                  (logicalCaveWdt * m_cave_info.mCellWidth);
        dWidth = m_cave_info.mBorderWidth;
      } else {
        // Inner Cell Column
        dStartX =
            m_cave_info.mBorderWidth + ((sx - 1) * m_cave_info.mCellWidth);
        dWidth = m_cave_info.mCellWidth;
        isBorderColumn = false;
      }

      //
      // Fill the Rect
      //
      Cave::TileName tile_name = static_cast<Cave::TileName>(caveMap[sy][sx]);
      Vector2i tile = map_tilename_to_vector2i(tile_name);
      if (tile_name != Cave::FLOOR && tile_name != Cave::WALL) {
        tile.x *= m_cave_info.mCellWidth;
        tile.y *= m_cave_info.mCellHeight;
      }
      for (int dy = 0; dy < dHeight; ++dy) {
        for (int dx = 0; dx < dWidth; ++dx) {
          Vector2i pos(dStartX + dx, dStartY + dy);
          Vector2i t = tile;
          // If on border don't acdjust and assume same tile for all (since
          // border is not WxH cell, but an absolute). FOr some reason floor
          // needs to use -1,-1 so don't adjust. Otherwise use the WxH cell
          t.x += (isBorderColumn || (t.x < 0)) ? 0 : dx;
          t.y += (isBorderRow || (t.x < 0)) ? 0 : dy;
          pTileMap->set_cell(pos, layer, t);
        }
      }
    }
  }
}

// So can have static version that doesn't use m_xxx_tile
Vector2i GDCave::getAtlasCoords(int tile_name) {
  auto coords = Cave::Cave::getAtlasCoords(tile_name);
  return Vector2i(coords.x, coords.y);
}

Vector2i GDCave::map_tilename_to_vector2i(int tile_name) {
  auto coords = getAtlasCoords(tile_name);
  if (tile_name == Cave::FLOOR) return m_floor_tile;
  if (tile_name == Cave::WALL) return m_wall_tile;
  return coords;
}

void GDCave::setCell(TileMapLayer* pTileMap, int layer, int cx, int cy,
                     Vector2i tile) {
  int mapX = m_cave_info.mBorderWidth + (cx * m_cave_info.mCellWidth);
  int mapY = m_cave_info.mBorderHeight + (cy * m_cave_info.mCellHeight);
  for (int y = 0; y < m_cave_info.mCellHeight; ++y) {
    for (int x = 0; x < m_cave_info.mCellWidth; ++x) {
      Vector2i pos(mapX + x, mapY + y);
      // For some reason Need to use -1,-1 for floor
      Vector2 t = (tile.x < 0) ? tile : Vector2i(tile.x + x, tile.y + y);
      pTileMap->set_cell(pos, layer, t);
    }
  }
}
