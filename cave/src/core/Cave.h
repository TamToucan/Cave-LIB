#ifndef CAVE_H
#define CAVE_H

#include <cstddef>
#include <unordered_map>
#include <vector>

#include "CaveInfo.h"
#include "GenerationParams.h"
#include "TileTypes.h"

namespace Cave {

struct Vector2Hash {
  size_t operator()(const Vector2i& v) const {
    return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
  }
};

struct Vector2Equal {
  bool operator()(const Vector2i& lhs, const Vector2i& rhs) const {
    return lhs.x == rhs.x && lhs.y == rhs.y;
  }
};

using Vector2iIntMap =
    std::unordered_map<Vector2i, int, Vector2Hash, Vector2Equal>;
using IntVectorOfVector2iMap = std::unordered_map<int, std::vector<Vector2i>>;

class Cave {
  CaveInfo mInfo;
  GenerationParams mParams;

 public:
  Cave(CaveInfo& info, const GenerationParams& params);
  ~Cave();

  TileMap generate();

  // Return true if the cell is empty (not a wall). This is needed
  // for when corners have been rounded e.g. DEND_W is still "floor"
  static bool isEmpty(int tile) {
    return tile >= FLOOR_START && tile <= FLOOR_END;
  }
  static bool isEmpty(const TileMap& tileMap, int cx, int cy) {
    return isEmpty(getTile(tileMap, cx, cy));
  }

  static Vector2i getAtlasCoords(int tile);
  static int getAtlasIndex(int tile);

 private:
  void initialise(TileMap& tileMap);
  void runCellularAutomata(TileMap& tileMap);
  void fixUp(TileMap& tileMap);
  std::pair<Vector2iIntMap, IntVectorOfVector2iMap> findRooms(TileMap& tileMap);
  void joinRooms(TileMap& tileMap,
                 std::pair<Vector2iIntMap, IntVectorOfVector2iMap> floorMaps);
  void smooth(TileMap& tileMap);

  struct BorderWall {
    Vector2i floor1;
    Vector2i floor2;
    Vector2i dir;
    int room1;
    int room2;
    int thickness;
  };
  std::vector<BorderWall> detectBorderWalls(
      TileMap& tileMap,
      std::pair<Vector2iIntMap, IntVectorOfVector2iMap> floorMaps);
  std::vector<BorderWall>
  findMST_Kruskal(std::vector<Cave::BorderWall>& borderWalls,
                  std::vector<int> roomIds);

 public:
  // NOTE: Return IGNORE if out of bounds
  static TileName getTile(const TileMap& tileMap, int cx, int cy);
  static bool isTile(const TileMap& tileMap, int cx, int cy, int tile) {
    return getTile(tileMap, cx, cy) == tile;
  }
  static bool isWall(const TileMap& tileMap, int cx, int cy) {
    return isTile(tileMap, cx, cy, WALL);
  }
  static bool isFloor(const TileMap& tileMap, int cx, int cy) {
    return isTile(tileMap, cx, cy, FLOOR);
  }
  static void setCell(TileMap& tileMap, int x, int y, int tile);
  static Vector2i getMapPos(int x, int y);
};

}  // namespace Cave

#endif
