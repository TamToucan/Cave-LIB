#ifndef CAVE_H
#define CAVE_H

/**
 * @file Cave.h
 * @brief Top-level cave generator orchestrator.
 * @details Dispatches to a generator backend (Cellular, Voronoi, Heightmap,
 * DLA, BSP Tectonic) selected via GenerationParams::mCaveType, then runs the
 * shared post-processing pipeline: cellular-automata smoothing, room
 * detection, MST-based room joining via organic tunnels, and final edge/corner
 * smoothing. Output is a TileMap of TileName values.
 */

#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "CaveInfo.h"
#include "GenerationParams.h"
#include "TileTypes.h"

namespace Cave {

/// Hash functor for Vector2i — used as key for unordered_map of grid cells.
struct Vector2Hash {
  size_t operator()(const Vector2i &v) const {
    return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
  }
};

/// Equality functor for Vector2i — used alongside Vector2Hash.
struct Vector2Equal {
  bool operator()(const Vector2i &lhs, const Vector2i &rhs) const {
    return lhs.x == rhs.x && lhs.y == rhs.y;
  }
};

/// Map grid cell -> room id. Built by findRooms().
using Vector2iIntMap =
    std::unordered_map<Vector2i, int, Vector2Hash, Vector2Equal>;
/// Reverse map: room id -> list of floor cells belonging to that room.
using IntVectorOfVector2iMap = std::unordered_map<int, std::vector<Vector2i>>;

/**
 * @class Cave
 * @brief Generates a tile-based cave layout per GenerationParams.
 * @details Constructed with sizing info (CaveInfo) and generator settings
 * (GenerationParams); call generate() to produce the TileMap.
 */
class Cave {
  CaveInfo mInfo;
  GenerationParams mParams;
  /// Client hook run after backend generation and before room-join/smoothing.
  /// Lets the client stamp template tiles (ids >= CLIENT_TILE_BASE) into the
  /// map so the cave blends with them. Null = no-op.
  std::function<void(TileMap &)> mPostGenerateHook;

public:
  Cave(const CaveInfo &info, const GenerationParams &params);
  ~Cave();

  /// Run the full generation pipeline and return the finished TileMap.
  TileMap generate();

  /// Install the post-generate hook (see mPostGenerateHook). Null clears it.
  void setPostGenerateHook(std::function<void(TileMap &)> hook);

  // Return true if the cell is empty (not a wall). This is needed
  // for when corners have been rounded e.g. DEND_W is still "floor"
  static bool isEmpty(int tile) {
    return tile >= FLOOR_START && tile <= FLOOR_END;
  }
  static bool isEmpty(const TileMap &tileMap, int cx, int cy) {
    return isEmpty(getTile(tileMap, cx, cy));
  }

  /// Returns the atlas (column,row) coordinates for the given tile id.
  static Vector2i getAtlasCoords(int tile);
  /// Returns the flat atlas index for the given tile id.
  static int getAtlasIndex(int tile);

  /// True if at least one FLOOR cell exists in the generated map.
  bool hasFloorSpace(const TileMap &tileMap) const;
  /// Human-readable dump of the active GenerationParams (for debug overlays).
  std::string getParamsString() const;

private:
  /// Fill tileMap with the initial random/noise pattern per CaveType.
  void initialise(TileMap &tileMap);
  /// Run the configured cellular-automata generation steps.
  void runCellularAutomata(TileMap &tileMap);
  /// Patch isolated 1-cell walls/floors that confuse the smoother.
  void fixUp(TileMap &tileMap);
  /// Flood-fill rooms and return (cell->roomId, roomId->cells) maps.
  std::pair<Vector2iIntMap, IntVectorOfVector2iMap> findRooms(TileMap &tileMap);
  /// Build MST across rooms and carve connecting tunnels.
  void joinRooms(TileMap &tileMap,
                 std::pair<Vector2iIntMap, IntVectorOfVector2iMap> floorMaps);
  /// Apply final corner/edge smoothing pass via CaveSmoother.
  void smooth(TileMap &tileMap);

  /// Allocate a width*height TileMap initialised to WALL.
  static TileMap generateEmpty(int width, int height);

  /**
   * @struct BorderWall
   * @brief Candidate connection point between two rooms.
   * @details floor1/floor2 are the floor cells on each side of the wall; dir
   * is the normal across the wall; thickness counts the wall cells between
   * them.
   */
  struct BorderWall {
    Vector2i floor1;
    Vector2i floor2;
    Vector2i dir;
    int room1;
    int room2;
    int thickness;
  };
  /// Enumerate every wall segment that separates two distinct rooms.
  std::vector<BorderWall> detectBorderWalls(
      TileMap &tileMap,
      std::pair<Vector2iIntMap, IntVectorOfVector2iMap> floorMaps);
  /// Kruskal MST over BorderWalls so every room is reachable with minimum cost.
  std::vector<BorderWall>
  findMST_Kruskal(std::vector<Cave::BorderWall> &borderWalls,
                  std::vector<int> roomIds);

  /// Carve a single tunnel (straight or organic) across the given BorderWall.
  void drawTunnel(TileMap &tileMap, const BorderWall &node);

public:
  /// NOTE: Returns IGNORE if (cx,cy) out of bounds.
  static TileName getTile(const TileMap &tileMap, int cx, int cy);
  /// True if tile at (cx,cy) matches the given tile id.
  static bool isTile(const TileMap &tileMap, int cx, int cy, int tile) {
    return getTile(tileMap, cx, cy) == tile;
  }
  /// True if tile at (cx,cy) is WALL.
  static bool isWall(const TileMap &tileMap, int cx, int cy) {
    return isTile(tileMap, cx, cy, WALL);
  }
  /// True if tile at (cx,cy) is FLOOR (plain floor only, not rounded variants).
  static bool isFloor(const TileMap &tileMap, int cx, int cy) {
    return isTile(tileMap, cx, cy, FLOOR);
  }
  /// Bounds-checked write of tile into tileMap[y][x].
  static void setCell(TileMap &tileMap, int x, int y, int tile);
  /// Convert local cell coords to world-space map position (cell origin).
  static Vector2i getMapPos(int x, int y);
};

} // namespace Cave

#endif
