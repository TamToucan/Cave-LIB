#ifndef GD_CAVE_H
#define GD_CAVE_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/tile_map_layer.hpp>
#include <vector>

#include "core/CaveInfo.h"
#include "core/GenerationParams.h"
#include "core/TileTypes.h"

#if defined(WIN32) || defined(_WIN32)
#ifdef GDCAVE_EXPORTS
#define GDCAVE_API __declspec(dllexport)
#else
#define GDCAVE_API __declspec(dllimport)
#endif
#else
#define GDCAVE_API
#endif

namespace godot {

class GDCAVE_API GDCave : public Object {
  GDCLASS(GDCave, Object)

 protected:
  static void _bind_methods();

  Cave::CaveInfo m_cave_info;
  Cave::GenerationParams m_gen_params;
  std::vector<std::vector<int>> m_tile_map;

  godot::Vector2i m_floor_tile;
  godot::Vector2i m_wall_tile;

 public:
  GDCave();
  ~GDCave();

  GDCave* setCaveSize(godot::Vector2i caveSize);
  GDCave* setBorderCellSize(godot::Vector2i border);
  GDCave* setCellSize(godot::Vector2i cellSize);
  GDCave* setStartCell(int x, int y);
  GDCave* setFloor(godot::Vector2i floor);
  GDCave* setWall(godot::Vector2i wall);
  GDCave* setOctaves(int octaves);
  GDCave* setPerlin(bool usePerlin);
  GDCave* setWallChance(float wallChance);
  GDCave* setFreq(float freq);
  GDCave* setAmp(float amp);
  GDCave* setGenerations(const godot::Array& gens);

  void make_cave(TileMapLayer* pTileMap, int layer, int seed);

  static godot::Vector2i getAtlasCoords(int tile_name);

 private:
  godot::Vector2i map_tilename_to_vector2i(int tile_name);
  void copy_core_to_tilemap(TileMapLayer* pTileMap, int layer, const Cave::TileMap& caveMap);
  void setCell(TileMapLayer* pTileMap, int layer, int x, int y, godot::Vector2i tile);
};

}  // namespace godot

#endif
