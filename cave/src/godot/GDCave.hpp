#ifndef GD_CAVE_H
#define GD_CAVE_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/tile_map_layer.hpp>
#include <vector>

#include "core/CaveInfo.h"
#include "core/GenerationParams.h"
#include "core/TileTypes.h"


namespace godot {

class GDCave : public Object {
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

  GDCave* setCaveSize(Vector2i caveSize);
  GDCave* setBorderCellSize(Vector2i border);
  GDCave* setCellSize(Vector2i cellSize);
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

 private:
  void copy_core_to_tilemap(TileMapLayer* pTileMap, int layer, const Cave::TileMap& caveMap);
  Vector2i map_tilename_to_vector2i(Cave::TileName tile_name);
  void setCell(TileMapLayer* pTileMap, int layer, int x, int y, Vector2i tile);
};

}  // namespace godot

#endif
