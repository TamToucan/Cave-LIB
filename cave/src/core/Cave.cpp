#include "Cave.h"

#include <algorithm>
#include <set>

#include "CaveSmoother.h"
#include "Debug.h"
#include "DisjointSets.h"
#include "PerlinNoise.h"
#include "RandSimple.h"
#include "RogueCave.hpp"
#include "SimplexNoise.h"
#include "TileTypes.h"

namespace Cave {

Cave::Cave(CaveInfo& info, const GenerationParams& params)
    : mInfo(info), mParams(params) {}

Cave::~Cave() {}

TileMap Cave::generate() {
  //
  // The TileMap is bordered with 1 tile wall. To make the loops easier? the X,Y
  // of the non-border corner is 0,0 and getMapPos translates it to 1,1.
  // Therefore -1,-1 is the top left corner of the border wall of TileMap.
  //
  TileMap tileMap(mInfo.mCaveHeight + 2,
                  std::vector<int>(mInfo.mCaveWidth + 2));

  initialise(tileMap);
  runCellularAutomata(tileMap);
  fixUp(tileMap);
  auto floorMaps = findRooms(tileMap);
  joinRooms(tileMap, floorMaps);
  smooth(tileMap);

  return tileMap;
}

void Cave::initialise(TileMap& tileMap) {
  RNG::RandSimple simple(mParams.seed);

  //
  // Make the border
  // - Top/Bottom
  //
  for (int cx = 0; cx < 2 + mInfo.mCaveWidth; ++cx) {
    setCell(tileMap, cx - 1, -1, WALL);
    setCell(tileMap, cx - 1, mInfo.mCaveHeight, WALL);
  }
  // - Left/Right
  for (int cy = 0; cy < 2 + mInfo.mCaveHeight; ++cy) {
    setCell(tileMap, -1, cy - 1, WALL);
    setCell(tileMap, mInfo.mCaveWidth, cy - 1, WALL);
  }

  //
  // Fill with random or perlin
  //
  const double W = mInfo.mCaveWidth - 1 + mParams.mAmp;
  const double H = mInfo.mCaveHeight - 1 + mParams.mAmp;
  double (*pf)(double, double, int) =
      mParams.mPerlin ? &Algo::getSNoise2 : &Algo::getNoise2;

  for (int cy = 0; cy < mInfo.mCaveHeight; ++cy) {
    for (int cx = 0; cx < mInfo.mCaveWidth; ++cx) {
      double x = cx / W * mParams.mFreq;
      double y = cy / H * mParams.mFreq;

      double n1 = mParams.mPerlin
                      ? (*pf)(x, y, mParams.mOctaves)
                      : simple.getFloat() - mParams.mWallChance;
      setCell(tileMap, cx, cy, (n1 < 0) ? WALL : FLOOR);
    }
  }
}

void Cave::runCellularAutomata(TileMap& tileMap) {
  if (!mParams.mGenerations.empty()) {
    // initialise the RogueCave grid from the TileMap
    PCG::RogueCave cave(mInfo.mCaveWidth, mInfo.mCaveHeight);
    std::vector<std::vector<int>>& gridIn = cave.getGrid();
    for (int cy = 0; cy < mInfo.mCaveHeight; ++cy) {
      for (int cx = 0; cx < mInfo.mCaveWidth; ++cx) {
        gridIn[cy][cx] = Cave::isWall(tileMap, cx, cy)
                             ? PCG::RogueCave::TILE_WALL
                             : PCG::RogueCave::TILE_FLOOR;
        LOG_DEBUG_CONT(
            ((gridIn[cy][cx] == PCG::RogueCave::TILE_FLOOR) ? ' ' : '#'));
      }
      LOG_DEBUG(" ");
    }

    // run the cellular automata
    for (const auto& gen : mParams.mGenerations) {
      Util::IntRange b3(gen.b3_min, gen.b3_max);
      Util::IntRange b5(gen.b5_min, gen.b5_max);
      Util::IntRange s3(gen.s3_min, gen.s3_max);
      Util::IntRange s5(gen.s5_min, gen.s5_max);
      cave.addGeneration(b3, b5, s3, s5, gen.reps);
    }
    std::vector<std::vector<int>>& gridOut = cave.generate();

    // Copy the RogueCave grid back to the TileMap
    LOG_DEBUG("-----GRID OUT-----");
    for (int cy = 0; cy < mInfo.mCaveHeight; ++cy) {
      for (int cx = 0; cx < mInfo.mCaveWidth; ++cx) {
        auto tile =
            (gridOut[cy][cx] == PCG::RogueCave::TILE_WALL) ? WALL : FLOOR;
        setCell(tileMap, cx, cy, tile);
        LOG_DEBUG_CONT(
            ((gridOut[cy][cx] == PCG::RogueCave::TILE_FLOOR) ? ' ' : '#'));
      }
      LOG_DEBUG(" ");
    }
  }
}

void Cave::fixUp(TileMap& tileMap) {
  std::vector<Vector2i> walls;
  std::vector<Vector2i> floors;
  for (int lp = 0; lp < 10; ++lp) {
    for (int cy = 0; cy < mInfo.mCaveHeight; ++cy) {
      for (int cx = 0; cx < mInfo.mCaveWidth; ++cx) {
        int DIAG = 0;
        int NSEW = 0;

        if (Cave::isWall(tileMap, cx - 1, cy - 1))
          DIAG += 1;
        if (Cave::isWall(tileMap, cx, cy - 1))
          NSEW += 1;
        if (Cave::isWall(tileMap, cx + 1, cy - 1))
          DIAG += 2;
        if (Cave::isWall(tileMap, cx + 1, cy))
          NSEW += 2;
        if (Cave::isWall(tileMap, cx - 1, cy))
          NSEW += 8;
        if (Cave::isWall(tileMap, cx - 1, cy + 1))
          DIAG += 8;
        if (Cave::isWall(tileMap, cx, cy + 1))
          NSEW += 4;
        if (Cave::isWall(tileMap, cx + 1, cy + 1))
          DIAG += 4;

        LOG_DEBUG(cx << "," << cy << " D:" << DIAG << " N:" << NSEW
                     << " W:" << Cave::isWall(tileMap, cx, cy));

        if (Cave::isWall(tileMap, cx, cy)) {
          if (((DIAG & 0b0001) != 0) && ((NSEW & 0b1001) == 0))
            floors.push_back({cx, cy});
          if (((DIAG & 0b0010) != 0) && ((NSEW & 0b0011) == 0))
            floors.push_back({cx, cy});
          if (((DIAG & 0b0100) != 0) && ((NSEW & 0b0110) == 0))
            floors.push_back({cx, cy});
          if (((DIAG & 0b1000) != 0) && ((NSEW & 0b1100) == 0))
            floors.push_back({cx, cy});
        } else if ((DIAG == 0b1111) && (NSEW == 0b1111)) {
          walls.push_back({cx, cy});
          LOG_DEBUG("ADDWALL: " << cx << "," << cy << " D:" << DIAG
                                << " N:" << NSEW);
        }
      }
    }
    LOG_DEBUG("WALLS: " << walls.size() << " FLOORS: " << floors.size());
    if (walls.empty() && floors.empty())
      return;
    for (Vector2i corner : walls) {
      LOG_DEBUG("WALL: " << corner.x << "," << corner.y);
      setCell(tileMap, corner.x, corner.y, WALL);
    }
    for (Vector2i corner : floors) {
      LOG_DEBUG("FLOOR: " << corner.x << "," << corner.y);
      setCell(tileMap, corner.x, corner.y, FLOOR);
    }
    walls.clear();
    floors.clear();
  }
}

std::pair<Vector2iIntMap, IntVectorOfVector2iMap>
Cave::findRooms(TileMap& tileMap) {
  Algo::DisjointSets<Vector2i> floors;
  static const std::vector<Vector2i> directions = {
      {0, 1}, {1, 0}, {0, -1}, {-1, 0}};
  LOG_DEBUG("----FIND ROOMS----");

  for (int cx = 0; cx < mInfo.mCaveWidth; ++cx) {
    for (int cy = 0; cy < mInfo.mCaveHeight; ++cy) {
      if (isFloor(tileMap, cx, cy)) {
        floors.addElement({cx, cy});
        LOG_DEBUG("FLOOR: " << cx << "," << cy);
      }
    }
  }

  for (int cx = 0; cx < mInfo.mCaveWidth; ++cx) {
    for (int cy = 0; cy < mInfo.mCaveHeight; ++cy) {
      if (isFloor(tileMap, cx, cy)) {
        for (const Vector2i& dir : directions) {
          int nx = cx + dir.x;
          int ny = cy + dir.y;
          if ((nx >= 0 && nx < mInfo.mCaveWidth) &&
              (ny >= 0 && ny < mInfo.mCaveHeight)) {
            if (isFloor(tileMap, nx, ny)) {
              int i1 = floors.findSet({cx, cy});
              int i2 = floors.findSet({nx, ny});
              LOG_DEBUG("JOIN: " << cx << "," << cy << " nxy:" << nx << ","
                                 << ny << " " << i1 << "->" << i2);
              floors.joinSets(i1, i2);
            }
          }
        }
      }
    }
  }

  Vector2iIntMap grid_to_set;
  IntVectorOfVector2iMap set_to_cells;

  for (int cx = 0; cx < mInfo.mCaveWidth; ++cx) {
    for (int cy = 0; cy < mInfo.mCaveHeight; ++cy) {
      if (isFloor(tileMap, cx, cy)) {
        Vector2i current = {cx, cy};
        int rootId = floors.findSet(current);
        grid_to_set[current] = rootId;
        set_to_cells[rootId].push_back(current);
        LOG_DEBUG("xy: " << cx << "," << cy << " <=> " << rootId);
      }
    }
  }

  return std::pair(grid_to_set, set_to_cells);
}

void Cave::joinRooms(
    TileMap& tileMap,
    std::pair<Vector2iIntMap, IntVectorOfVector2iMap> floorMaps) {
  std::vector<Cave::BorderWall> borderWalls =
      detectBorderWalls(tileMap, floorMaps);
  IntVectorOfVector2iMap roomToFloorsMap = floorMaps.second;

  LOG_DEBUG("----JOIN ROOMS----");
  for (int y = 0; y < tileMap.size(); ++y) {
    for (int x = 0; x < tileMap[0].size(); ++x) {
      LOG_DEBUG_CONT(((tileMap[y][x] == FLOOR) ? ' ' : '#'));
    }
    LOG_DEBUG("");
  }

  std::vector<int> roomIds;
  for (auto p : roomToFloorsMap) {
    roomIds.push_back(p.first);
  }
  std::vector<Cave::BorderWall> mst = findMST_Kruskal(borderWalls, roomIds);
  for (auto& node : mst) {
    int wx = node.floor1.x + node.dir.x;
    int wy = node.floor1.y + node.dir.y;
    LOG_DEBUG_CONT("TUNNEL: " << wx << "," << wy << " dir: " << node.dir.x
                              << "," << node.dir.y
                              << " thick: " << node.thickness);
    for (int i = 0; i < node.thickness; ++i) {
      setCell(tileMap, wx, wy, SOLID);
      LOG_DEBUG_CONT("  " << wx << "," << wy);
      wx += node.dir.x;
      wy += node.dir.y;
    }
    LOG_DEBUG("");
  }
  LOG_DEBUG("----JOIN ROOMS END----");
  for (int y = 0; y < tileMap.size(); ++y) {
    for (int x = 0; x < tileMap[0].size(); ++x) {
      if (tileMap[y][x] == SOLID) {
        LOG_DEBUG_CONT('X');
        tileMap[y][x] = FLOOR;
      } else if (tileMap[y][x] == FLOOR) {
        LOG_DEBUG_CONT(' ');
      } else {
        LOG_DEBUG_CONT('#');
      }
    }
    LOG_DEBUG("");
  }
}

std::vector<Cave::BorderWall> Cave::detectBorderWalls(
    TileMap& tileMap,
    std::pair<Vector2iIntMap, IntVectorOfVector2iMap> floorMaps) {
  std::vector<BorderWall> borderWalls;
  Vector2iIntMap floorToRoomMap = floorMaps.first;
  IntVectorOfVector2iMap roomsMap = floorMaps.second;

  LOG_DEBUG("----DETECT BORDER WALLS----");
  LOG_DEBUG("ROOMS: " << roomsMap.size());
  for (const auto& [roomID, tiles] : roomsMap) {
    LOG_DEBUG_CONT("Tiles: " << tiles.size());
    for (const auto& tile : tiles) {
      LOG_DEBUG_CONT(" " << tile.x << "," << tile.y);
    }
    LOG_DEBUG(" ID: " << roomID);
  }

  // EVERYTHING BEFORE HERE IS THE SAME
  // (the roomsMap is a map and is different order, but same content)
  // Hmmmmmm, maybe try sorting
  // But delete everything between DETECT and JOIN then sort and compare (ignore
  // space)
  //   WORK: 10003=> CHECK: xy:-1,10 thick:4 floor:0
  //   FAIL: 10003=> CHECK: xy:-2,10 thick:5 floor:0
  // also this appears
  //   WORK: 1013==> ADJROOM: xy:16, 1 tile : 12, 1 r: 38 r2: 276
  //   WORK: 1013=> BWALL: xy:16,1 tile: 12,1 r1: 38 r2: 276 thick: 3 wallDir:
  //   1,0
  // then fail gets
  //   FAIL: 1015==> ADJROOM: xy:12,1 tile: 16,1 r: 276 r2: 38
  //   FAIL: 1015=> BWALL: xy:12,1 tile: 16,1 r1: 38 r2: 276 thick: 3 wallDir:
  //   -1,0 FAIL: 1015=> CHECK: xy:12,1 thick:3 floor:1
  //
  std::vector<int> checkedRooms;
  // FIX: Iterate over rooms in a deterministic order (sorted by roomID)
  // The original iteration over `roomsMap` (unordered_map) caused
  // non-deterministic wall detection order.
  std::vector<int> sortedRoomIds;
  sortedRoomIds.reserve(roomsMap.size());
  for (const auto& pair : roomsMap) {
    sortedRoomIds.push_back(pair.first);
  }
  std::sort(sortedRoomIds.begin(), sortedRoomIds.end());

  for (int roomID : sortedRoomIds) {
    const auto& tiles = roomsMap[roomID];
    checkedRooms.push_back(roomID);
    for (const auto& tile : tiles) {
      for (const auto& dir :
           {Vector2i{-1, 0}, Vector2i{1, 0}, Vector2i{0, -1}, Vector2i{0, 1}}) {
        int cx = tile.x + dir.x;
        int cy = tile.y + dir.y;
        unsigned long id = cy * 1000 + cx;
        LOG_DEBUG(id << " CHECK: xy:" << cx << "," << cy << " wall:"
                     << isWall(tileMap, cx, cy) << "(tile:" << tile.x << ","
                     << tile.y << " dir:" << dir.x << "," << dir.y << ")");
        if (isWall(tileMap, cx, cy)) {
          std::set<int> adjacentRooms;
          adjacentRooms.insert(roomID);

          int thickness = 0;
          while (isWall(tileMap, cx, cy)) {
            cx += dir.x;
            cy += dir.y;
            thickness++;
          }

          LOG_DEBUG(id << "=> CHECK: xy:" << cx << "," << cy << " thick:"
                       << thickness << " floor:" << isFloor(tileMap, cx, cy));
          if (isFloor(tileMap, cx, cy)) {
            auto it = floorToRoomMap.find({cx, cy});
            if (it != floorToRoomMap.end()) {
              int otherRoomID = it->second;
              if (std::find(checkedRooms.begin(), checkedRooms.end(),
                            otherRoomID) == checkedRooms.end()) {
                LOG_DEBUG(id << "==> ADJROOM: xy:" << cx << "," << cy
                             << " tile: " << tile.x << "," << tile.y
                             << " r: " << roomID << " r2: " << otherRoomID);
                adjacentRooms.insert(otherRoomID);
              }
            }
          }

          if (adjacentRooms.size() == 2) {
            auto it = adjacentRooms.begin();
            int room1 = *it++;
            int room2 = *it;
            LOG_DEBUG(id << "=> BWALL: xy:" << cx << "," << cy << " tile: "
                         << tile.x << "," << tile.y << " r1: " << room1
                         << " r2: " << room2 << " thick: " << thickness
                         << " wallDir: " << dir.x << "," << dir.y);
            borderWalls.push_back(
                {tile, {cx, cy}, dir, room1, room2, thickness});
          }
        }
      }
    }
  }
  return borderWalls;
}

std::vector<Cave::BorderWall>
Cave::findMST_Kruskal(std::vector<Cave::BorderWall>& borderWalls,
                      std::vector<int> roomIds) {
  std::vector<BorderWall> mst;
  Algo::DisjointSets<int> dsu;
  const int numRooms = roomIds.size();

  for (int i : roomIds) {
    dsu.addElement(i);
  }

  // FIX: Use a fully deterministic comparator.
  // Previous comparator only used thickness, which is not unique.
  std::sort(borderWalls.begin(), borderWalls.end(),
            [](const BorderWall& a, const BorderWall& b) {
              if (a.thickness != b.thickness)
                return a.thickness < b.thickness;
              if (a.room1 != b.room1)
                return a.room1 < b.room1;
              if (a.room2 != b.room2)
                return a.room2 < b.room2;
              if (a.dir.x != b.dir.x)
                return a.dir.x < b.dir.x;
              if (a.dir.y != b.dir.y)
                return a.dir.y < b.dir.y;
              if (a.floor1.x != b.floor1.x)
                return a.floor1.x < b.floor1.x;
              return a.floor1.y < b.floor1.y;
            });
  LOG_INFO("=== findMST: " << borderWalls.size() << " rooms: " << numRooms);

  for (const BorderWall& wall : borderWalls) {
    int setU = dsu.findSet(wall.room1);
    int setV = dsu.findSet(wall.room2);
    if (setU != setV) {
      mst.push_back(wall);
      dsu.joinSets(setU, setV);
      LOG_DEBUG(" JOIN " << setU << " " << setV);
    }
    if (mst.size() == numRooms - 1)
      break;
  }

  LOG_INFO("DONE MST: " << mst.size());
  for (auto& node : mst) {
    LOG_DEBUG("BORDER: r1 = " << node.room1 << " r2 = " << node.room2
                              << " thick = " << node.thickness
                              << " wall=" << node.dir.x << "," << node.dir.y);
  }
  return mst;
}

void Cave::smooth(TileMap& tileMap) {
  if (!mInfo.mSmoothing) {
    return;
  }
  CaveSmoother smoother(tileMap, mInfo);
  smoother.smooth();
}

TileName Cave::getTile(const TileMap& tileMap, int cx, int cy) {
  Vector2i mapPos = getMapPos(cx, cy);
  if (mapPos.y >= 0 && mapPos.y < (int)tileMap.size() &&
      mapPos.x >= 0 && mapPos.x < (int)tileMap[0].size()) {
    return static_cast<TileName>(tileMap[mapPos.y][mapPos.x]);
  }
  return IGNORE;
}

Vector2i Cave::getMapPos(int cx, int cy) { return {1 + cx, 1 + cy}; }

void Cave::setCell(TileMap& tileMap, int x, int y, int tile) {
  Vector2i mapPos = getMapPos(x, y);
  tileMap[mapPos.y][mapPos.x] = tile;
}

}  // namespace Cave
