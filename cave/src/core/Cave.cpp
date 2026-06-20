#include <algorithm>
#include <set>
#include <sstream> // Moved from getParamsString() to top
#include <string>  // Added as per instruction
#include <unordered_map>

#include "Cave.h"
#include "CaveBspTectonic.h"
#include "CaveDLA.h"
#include "CaveHeightmap.h"
#include "CaveSmoother.h"
#include "Debug.h"
#include "DisjointSets.h"
#include "RandSimple.h"
#include "RandUniversal.h" // Added as per instruction
#include "RogueCave.hpp"
#include "SimplexNoise.h"
#include "TileTypes.h"
#include "VoronoiNoise.h"

namespace Cave {

Cave::Cave(const CaveInfo &info, const GenerationParams &params)
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

  LOG_INFO("Cave::getParamsString() " << getParamsString());
  initialise(tileMap);

  TileMap genMap;
  switch (mParams.mCaveType) {
  case CaveType::VORONOI:
    genMap =
        VoronoiNoise::generate(mInfo.mCaveWidth, mInfo.mCaveHeight, mParams);
    break;
  case CaveType::HEIGHTMAP:
    genMap =
        CaveHeightmap::generate(mInfo.mCaveWidth, mInfo.mCaveHeight, mParams);
    break;
  case CaveType::DLA:
    genMap = CaveDLA::generate(mInfo.mCaveWidth, mInfo.mCaveHeight, mParams);
    break;
  case CaveType::BSP_TECTONIC:
    genMap =
        CaveBspTectonic::generate(mInfo.mCaveWidth, mInfo.mCaveHeight, mParams);
    break;
  case CaveType::EMPTY:
    genMap = generateEmpty(mInfo.mCaveWidth, mInfo.mCaveHeight);
    break;
  default:
    runCellularAutomata(tileMap);
    break;
  }

  if (!genMap.empty()) {
    for (int cy = 0; cy < mInfo.mCaveHeight; ++cy) {
      for (int cx = 0; cx < mInfo.mCaveWidth; ++cx) {
        setCell(tileMap, cx, cy, genMap[cy][cx]);
      }
    }
  }

  if (!hasFloorSpace(tileMap)) {
    LOG_WARNING("Cave generation produced no floor space!");
    LOG_WARNING(getParamsString());
    LOG_WARNING("Falling back to a simple rectangular room.");
    for (int cy = 0; cy < mInfo.mCaveHeight; ++cy) {
      for (int cx = 0; cx < mInfo.mCaveWidth; ++cx) {
        setCell(tileMap, cx, cy, FLOOR);
      }
    }
  }

  fixUp(tileMap);
  auto floorMaps = findRooms(tileMap);
  joinRooms(tileMap, floorMaps);
  smooth(tileMap);

  return tileMap;
}

void Cave::initialise(TileMap &tileMap) {
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
  // Fill with random or simplex noise
  //
  const double W = mInfo.mCaveWidth - 1 + mParams.cellular.mAmp;
  const double H = mInfo.mCaveHeight - 1 + mParams.cellular.mAmp;
  // Seed-derived z-slice so Simplex output varies per seed (matches
  // CaveHeightmap pattern)
  const double zOffset = (mParams.seed % 10000) / 100.0;

  for (int cy = 0; cy < mInfo.mCaveHeight; ++cy) {
    for (int cx = 0; cx < mInfo.mCaveWidth; ++cx) {
      double x = cx / W * mParams.cellular.mFreq;
      double y = cy / H * mParams.cellular.mFreq;

      double n1 =
          mParams.cellular.mPerlin
              ? Algo::getSNoise3(x, y, zOffset, mParams.cellular.mOctaves)
              : simple.getFloat() - mParams.cellular.mWallChance;
      setCell(tileMap, cx, cy, (n1 < 0) ? WALL : FLOOR);
    }
  }
}

void Cave::runCellularAutomata(TileMap &tileMap) {
  if (!mParams.cellular.mGenerations.empty()) {
    // initialise the RogueCave grid from the TileMap
    PCG::RogueCave cave(mInfo.mCaveWidth, mInfo.mCaveHeight);
    std::vector<std::vector<int>> &gridIn = cave.getGrid();
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
    for (const auto &gen : mParams.cellular.mGenerations) {
      Util::IntRange b3(gen.b3_min, gen.b3_max);
      Util::IntRange b5(gen.b5_min, gen.b5_max);
      Util::IntRange s3(gen.s3_min, gen.s3_max);
      Util::IntRange s5(gen.s5_min, gen.s5_max);
      cave.addGeneration(b3, b5, s3, s5, gen.reps);
    }
    std::vector<std::vector<int>> &gridOut = cave.generate();

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

void Cave::fixUp(TileMap &tileMap) {
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
        }
      }
    }
    if (walls.empty() && floors.empty())
      return;
    for (Vector2i corner : walls) {
      setCell(tileMap, corner.x, corner.y, WALL);
    }
    for (Vector2i corner : floors) {
      setCell(tileMap, corner.x, corner.y, FLOOR);
    }
    walls.clear();
    floors.clear();
  }
}

std::pair<Vector2iIntMap, IntVectorOfVector2iMap>
Cave::findRooms(TileMap &tileMap) {
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
        for (const Vector2i &dir : directions) {
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
      }
    }
  }

  return std::pair(grid_to_set, set_to_cells);
}

namespace {

void carveCircle(TileMap &tileMap, int cx, int cy, float radius, int maxWidth,
                 int maxHeight) {
  int r = static_cast<int>(std::ceil(radius));
  for (int y = cy - r; y <= cy + r; ++y) {
    for (int x = cx - r; x <= cx + r; ++x) {
      if (x >= 0 && x < maxWidth && y >= 0 && y < maxHeight) {
        float dx = static_cast<float>(x - cx);
        float dy = static_cast<float>(y - cy);
        if (dx * dx + dy * dy <= radius * radius) {
          Cave::setCell(tileMap, x, y, SOLID);
        }
      }
    }
  }
}

} // namespace

void Cave::drawTunnel(TileMap &tileMap, const BorderWall &node) {
  int fx1 = node.floor1.x;
  int fy1 = node.floor1.y;
  int fx2 = node.floor2.x;
  int fy2 = node.floor2.y;

  float dx = static_cast<float>(fx2 - fx1);
  float dy = static_cast<float>(fy2 - fy1);
  float dist = std::sqrt(dx * dx + dy * dy);

  if (dist <= 0)
    return; // safety

  float stepX = dx / dist;
  float stepY = dy / dist;
  int steps = static_cast<int>(std::round(dist));

  if (node.thickness >= mParams.tunnel.mMinLengthForOrganic) {
    // Perlin Organic Tunnel Logic
    float orthoX = -stepY;
    float orthoY = stepX;
    int prevDrawX = -1;
    int prevDrawY = -1;

    // Create a unique seed for this specific tunnel
    // We strictly modulo the integer seed BEFORE converting to float to
    // prevent float precision collapse inside SimplexNoise which
    // mathematically returns 0 for floats > ~10^8
    int safeSeed = std::abs(mParams.seed) % 10000;
    float tunnelSeed =
        static_cast<float>(safeSeed) + (node.room1 * 7) + (node.room2 * 13);

    LOG_INFO("DRAW-TUNNEL:\n  SEED: "
             << tunnelSeed << "\n  FLOOR1 XY: " << node.floor1.x << ","
             << node.floor1.y << "\n  FLOOR2 XY: " << node.floor2.x << ","
             << node.floor2.y << "\n  THICKNESS: " << node.thickness);
    for (int i = 0; i <= steps; ++i) {
      float pct = (steps > 0) ? (static_cast<float>(i) / steps) : 0.5f;
      float actualX = fx1 + (stepX * i);
      float actualY = fy1 + (stepY * i);

      float envelope = std::sin(pct * 3.14159265f);

      float wiggleOffset =
          Algo::getSNoise2(
              actualX * mParams.tunnel.mWiggleFrequency,
              actualY * mParams.tunnel.mWiggleFrequency + tunnelSeed, 1) *
          mParams.tunnel.mWiggleAmplitude * envelope;

      float radius =
          1.0f +
          std::abs(Algo::getSNoise2(
              actualX * mParams.tunnel.mWidthPulseFrequency + 100.0f,
              actualY * mParams.tunnel.mWidthPulseFrequency + tunnelSeed, 1)) *
              mParams.tunnel.mWidthPulseAmplitude;

      int drawX = std::clamp(
          static_cast<int>(std::round(actualX + orthoX * wiggleOffset)), 0,
          mInfo.mCaveWidth - 1);
      int drawY = std::clamp(
          static_cast<int>(std::round(actualY + orthoY * wiggleOffset)), 0,
          mInfo.mCaveHeight - 1);

      if (prevDrawX == -1) {
        carveCircle(tileMap, drawX, drawY, radius, mInfo.mCaveWidth,
                    mInfo.mCaveHeight);
      } else {
        int bdx = std::abs(drawX - prevDrawX);
        int bdy = -std::abs(drawY - prevDrawY);
        int sx = prevDrawX < drawX ? 1 : -1;
        int sy = prevDrawY < drawY ? 1 : -1;
        int err = bdx + bdy;
        int currentX = prevDrawX;
        int currentY = prevDrawY;
        while (true) {
          carveCircle(tileMap, currentX, currentY, radius, mInfo.mCaveWidth,
                      mInfo.mCaveHeight);
          if (currentX == drawX && currentY == drawY) {
            break;
          }
          int e2 = 2 * err;
          if (e2 >= bdy) {
            err += bdy;
            currentX += sx;
          }
          if (e2 <= bdx) {
            err += bdx;
            currentY += sy;
          }
        }
      }
      prevDrawX = drawX;
      prevDrawY = drawY;
    }
  } else {
    // Standard Straight Tunnel via Bresenham
    int idx = std::abs(fx2 - fx1);
    int idy = -std::abs(fy2 - fy1);
    int sx = fx1 < fx2 ? 1 : -1;
    int sy = fy1 < fy2 ? 1 : -1;
    int err = idx + idy;
    int currentX = fx1;
    int currentY = fy1;
    while (true) {
      setCell(tileMap, currentX, currentY, SOLID);
      if (currentX == fx2 && currentY == fy2)
        break;
      int e2 = 2 * err;
      if (e2 >= idy) {
        err += idy;
        currentX += sx;
      }
      if (e2 <= idx) {
        err += idx;
        currentY += sy;
      }
    }
  }
}

void Cave::joinRooms(
    TileMap &tileMap,
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
  for (auto &node : mst) {
    LOG_DEBUG_CONT("TUNNEL: " << node.floor1.x << "," << node.floor1.y << " -> "
                              << node.floor2.x << "," << node.floor2.y
                              << " thick/dist: " << node.thickness);
    drawTunnel(tileMap, node);
    LOG_DEBUG("");
  }

  // --- Extra Connections (non-MST optional loops) ---
  if (mParams.tunnel.mExtraConnections > 0) {
    // Build a set of room pairs already connected by the MST
    std::set<std::pair<int, int>> mstPairs;
    for (const auto &node : mst) {
      mstPairs.insert(
          {std::min(node.room1, node.room2), std::max(node.room1, node.room2)});
    }

    // Group all non-MST candidate edges by their constituent rooms
    std::unordered_map<int, std::vector<const Cave::BorderWall *>> roomExtras;
    for (const auto &bw : borderWalls) {
      auto key = std::make_pair(std::min(bw.room1, bw.room2),
                                std::max(bw.room1, bw.room2));
      if (mstPairs.count(key) == 0) {
        roomExtras[bw.room1].push_back(&bw);
        roomExtras[bw.room2].push_back(&bw);
      }
    }

    // Use the project's deterministic RNG (matches CaveBspTectonic/CaveDLA
    // pattern). Seeded from mParams.seed so results are fully deterministic.
    RNG::RandUniversal rng(static_cast<unsigned>(mParams.seed));
    std::set<std::pair<int, int>> drawnExtras;
    const int maxDist = mParams.tunnel.mExtraConnectionMaxDist;

    for (auto &[roomId, candidates] : roomExtras) {
      // Per-room probability gate
      if (static_cast<float>(rng.getFloat()) >
          mParams.tunnel.mExtraConnectionChance)
        continue;

      // Sort by Euclidean distance ascending so extra connections prefer the
      // nearest non-MST neighbours first, preventing long cross-map tunnels.
      std::sort(candidates.begin(), candidates.end(),
                [](const Cave::BorderWall *a, const Cave::BorderWall *b) {
                  return a->thickness < b->thickness;
                });

      int drawn = 0;
      for (const Cave::BorderWall *bw : candidates) {
        if (drawn >= mParams.tunnel.mExtraConnections)
          break;
        // Distance cap: list is sorted so once we exceed maxDist we are done.
        if (maxDist > 0 && bw->thickness > maxDist)
          break;
        auto key = std::make_pair(std::min(bw->room1, bw->room2),
                                  std::max(bw->room1, bw->room2));
        if (drawnExtras.count(key))
          continue;
        drawnExtras.insert(key);
        drawTunnel(tileMap, *bw);
        drawn++;
      }
    }
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
    TileMap &tileMap,
    std::pair<Vector2iIntMap, IntVectorOfVector2iMap> floorMaps) {
  std::vector<BorderWall> borderWalls;
  IntVectorOfVector2iMap roomsMap = floorMaps.second;

  LOG_DEBUG("----DETECT BORDER WALLS (EUCLIDEAN)----");

  // Sort room IDs for deterministic iteration
  std::vector<int> roomIds;
  roomIds.reserve(roomsMap.size());
  for (const auto &pair : roomsMap) {
    roomIds.push_back(pair.first);
  }
  std::sort(roomIds.begin(), roomIds.end());

  // Cache Boundary Floors for each Room
  // Boundary Floor = a FLOOR tile that has at least one adjacent WALL (so it's
  // on the edge of the room)
  std::unordered_map<int, std::vector<Vector2i>> roomBoundaries;
  for (int roomID : roomIds) {
    for (const auto &tile : roomsMap[roomID]) {
      // Check 8-way neighbors for walls or out-of-bounds
      bool isBoundary = false;
      for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
          if (dx == 0 && dy == 0)
            continue;
          int nx = tile.x + dx;
          int ny = tile.y + dy;
          if (isWall(tileMap, nx, ny)) {
            isBoundary = true;
            break;
          }
        }
        if (isBoundary)
          break;
      }

      if (isBoundary) {
        roomBoundaries[roomID].push_back(tile);
      }
    }
    LOG_DEBUG("Room " << roomID << " has " << roomBoundaries[roomID].size()
                      << " boundary tiles.");
  }

  // Find the shortest straight line connection between every pair of rooms
  for (size_t i = 0; i < roomIds.size(); ++i) {
    for (size_t j = i + 1; j < roomIds.size(); ++j) {
      int roomA = roomIds[i];
      int roomB = roomIds[j];

      const auto &boundsA = roomBoundaries[roomA];
      const auto &boundsB = roomBoundaries[roomB];

      if (boundsA.empty() || boundsB.empty())
        continue;

      float minDistanceSq = std::numeric_limits<float>::max();
      Vector2i bestA = {0, 0};
      Vector2i bestB = {0, 0};

      for (const auto &a : boundsA) {
        for (const auto &b : boundsB) {
          float dx = static_cast<float>(a.x - b.x);
          float dy = static_cast<float>(a.y - b.y);
          float distSq = (dx * dx) + (dy * dy);

          if (distSq < minDistanceSq) {
            minDistanceSq = distSq;
            bestA = a;
            bestB = b;
          }
        }
      }

      BorderWall wall;
      wall.floor1 = bestA;
      wall.floor2 = bestB;
      // We don't use dir explicitly anymore for drawing, but we can set it to
      // the nominal sign vector just so the Kruskal tie-breaker has some data.
      wall.dir = {(bestB.x > bestA.x) ? 1 : ((bestB.x < bestA.x) ? -1 : 0),
                  (bestB.y > bestA.y) ? 1 : ((bestB.y < bestA.y) ? -1 : 0)};
      wall.room1 = roomA;
      wall.room2 = roomB;
      wall.thickness = static_cast<int>(std::round(std::sqrt(minDistanceSq)));
      borderWalls.push_back(wall);
    }
  }

  return borderWalls;
}

std::vector<Cave::BorderWall>
Cave::findMST_Kruskal(std::vector<Cave::BorderWall> &borderWalls,
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
            [](const BorderWall &a, const BorderWall &b) {
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

  for (const BorderWall &wall : borderWalls) {
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
  for (auto &node : mst) {
    LOG_DEBUG("BORDER: r1 = " << node.room1 << " r2 = " << node.room2
                              << " thick = " << node.thickness
                              << " wall=" << node.dir.x << "," << node.dir.y);
  }
  return mst;
}

void Cave::smooth(TileMap &tileMap) {
  CaveSmoother smoother(tileMap, mInfo);
  smoother.smooth();
}

TileName Cave::getTile(const TileMap &tileMap, int cx, int cy) {
  Vector2i mapPos = getMapPos(cx, cy);
  if (mapPos.y >= 0 && mapPos.y < (int)tileMap.size() && mapPos.x >= 0 &&
      mapPos.x < (int)tileMap[0].size()) {
    return static_cast<TileName>(tileMap[mapPos.y][mapPos.x]);
  }
  return IGNORE;
}

Vector2i Cave::getMapPos(int cx, int cy) { return {1 + cx, 1 + cy}; }

void Cave::setCell(TileMap &tileMap, int x, int y, int tile) {
  Vector2i mapPos = getMapPos(x, y);
  tileMap[mapPos.y][mapPos.x] = tile;
}

constexpr int ATLASWITDTH = 8;
Vector2i Cave::getAtlasCoords(int tile) {
  int idx = getAtlasIndex(tile);
  return {idx % ATLASWITDTH, idx / ATLASWITDTH};
}

int Cave::getAtlasIndex(int tile) {
  // Helper lambda to calculate index: Row * ATLASWITDTH + Col
  // Assuming 0,0 is Top-Left
  auto Idx = [](int col, int row) { return (row * ATLASWITDTH) + col; };

  switch (tile) {
  case TileName::FLOOR:
    return Idx(0, 7);
  case TileName::WALL:
    return Idx(1, 7);

  case TileName::T45a:
    return Idx(2, 6);
  case TileName::T45b:
    return Idx(3, 6);
  case TileName::T45c:
    return Idx(0, 6);
  case TileName::T45d:
    return Idx(1, 6);

  case TileName::V60a1:
    return Idx(2, 3);
  case TileName::V60a2:
    return Idx(2, 4);
  case TileName::V60b1:
    return Idx(1, 3);
  case TileName::V60b2:
    return Idx(1, 4);
  case TileName::V60c1:
    return Idx(3, 4);
  case TileName::V60c2:
    return Idx(3, 3);
  case TileName::V60d1:
    return Idx(0, 4);
  case TileName::V60d2:
    return Idx(0, 3);

  case TileName::H30a1:
    return Idx(2, 5);
  case TileName::H30a2:
    return Idx(3, 5);
  case TileName::H30b1:
    return Idx(7, 5);
  case TileName::H30b2:
    return Idx(6, 5);
  case TileName::H30c1:
    return Idx(1, 5);
  case TileName::H30c2:
    return Idx(0, 5);
  case TileName::H30d1:
    return Idx(4, 5);
  case TileName::H30d2:
    return Idx(5, 5);

  case TileName::END_N:
    return Idx(4, 6);
  case TileName::END_S:
    return Idx(6, 6);
  case TileName::END_E:
    return Idx(5, 6);
  case TileName::END_W:
    return Idx(7, 6);

  case TileName::DEND_N:
    return Idx(4, 7);
  case TileName::DEND_E:
    return Idx(5, 7);
  case TileName::DEND_S:
    return Idx(6, 7);
  case TileName::DEND_W:
    return Idx(7, 7);

  case TileName::CORNR_A:
    return Idx(4, 4);
  case TileName::CORNR_B:
    return Idx(5, 4);
  case TileName::CORNR_C:
    return Idx(6, 4);
  case TileName::CORNR_D:
    return Idx(7, 4);

  case TileName::SINGLE:
    return Idx(3, 7);

  case TileName::T45a2CT:
    return Idx(2, 0);
  case TileName::T45b2CT:
    return Idx(3, 0);
  case TileName::T45c2CT:
    return Idx(3, 1);
  case TileName::T45d2CT:
    return Idx(2, 1);

  case TileName::T45abCT:
    return Idx(4, 2);
  case TileName::T45adCT:
    return Idx(4, 1);
  case TileName::T45baCT:
    return Idx(5, 2);
  case TileName::T45bcCT:
    return Idx(5, 1);
  case TileName::T45cbCT:
    return Idx(6, 1);
  case TileName::T45cdCT:
    return Idx(6, 2);
  case TileName::T45daCT:
    return Idx(7, 1);
  case TileName::T45dcCT:
    return Idx(7, 2);

  case TileName::V60aCT:
    return Idx(1, 2);
  case TileName::V60bCT:
    return Idx(0, 2);
  case TileName::V60cCT:
    return Idx(2, 2);
  case TileName::V60dCT:
    return Idx(3, 2);

  case TileName::H30aCT:
    return Idx(0, 1);
  case TileName::H30bCT:
    return Idx(1, 1);
  case TileName::H30cCT:
    return Idx(1, 0);
  case TileName::H30dCT:
    return Idx(0, 0);

  case TileName::H30c1CT:
    return Idx(4, 0);
  case TileName::H30d1CT:
    return Idx(5, 0);
  case TileName::H30b1CT:
    return Idx(6, 0);
  case TileName::H30a1CT:
    return Idx(7, 0);

  case TileName::V60b1CT:
    return Idx(4, 3);
  case TileName::V60a1CT:
    return Idx(5, 3);
  case TileName::V60c1CT:
    return Idx(6, 3);
  case TileName::V60d1CT:
    return Idx(7, 3);

  case TileName::LADDER_DN:
    return Idx(5, 6);
  case TileName::LADDER_UP:
    return Idx(5, 7);

  default:
    return Idx(0, 7); // Default to FLOOR
  }
}

bool Cave::hasFloorSpace(const TileMap &tileMap) const {
  for (int cy = 0; cy < mInfo.mCaveHeight; ++cy) {
    for (int cx = 0; cx < mInfo.mCaveWidth; ++cx) {
      if (isFloor(tileMap, cx, cy)) {
        return true;
      }
    }
  }
  return false;
}

std::string Cave::getParamsString() const {
  std::stringstream ss;
  ss << "Generation Parameters:\n";
  ss << " - CaveType: " << static_cast<int>(mParams.mCaveType) << "\n";
  ss << " - Seed: " << mParams.seed << "\n";

  switch (mParams.mCaveType) {
  case CaveType::CELLULAR:
    ss << " CELLULAR\n";
    ss << " - Octaves: " << mParams.cellular.mOctaves << "\n";
    ss << " - Perlin: " << mParams.cellular.mPerlin << "\n";
    ss << " - WallChance: " << mParams.cellular.mWallChance << "\n";
    ss << " - Freq: " << mParams.cellular.mFreq << "\n";
    ss << " - Amp: " << mParams.cellular.mAmp << "\n";
    break;
  case CaveType::VORONOI:
    ss << " VORONOI\n";
    ss << " - NoiseScale: " << mParams.voronoi.mNoiseScale << "\n";
    ss << " - WarpStrength: " << mParams.voronoi.mWarpStrength << "\n";
    ss << " - VoronoiWeight: " << mParams.voronoi.mVoronoiWeight << "\n";
    ss << " - Threshold: " << mParams.voronoi.mThreshold << "\n";
    ss << " - InvertVoronoi: " << mParams.voronoi.mInvertVoronoi << "\n";
    break;
  case CaveType::HEIGHTMAP:
    ss << " HEIGHTMAP\n";
    ss << " - Octaves: " << mParams.heightmap.mOctaves << "\n";
    ss << " - Freq: " << mParams.heightmap.mFreq << "\n";
    ss << " - WaterLevelMin: " << mParams.heightmap.mWaterLevelMin << "\n";
    ss << " - WaterLevelMax: " << mParams.heightmap.mWaterLevelMax << "\n";
    break;
  case CaveType::DLA:
    ss << " DLA\n";
    ss << " - ParticleCount: " << mParams.dla.mParticleCount << "\n";
    ss << " - OpenAreaCount: " << mParams.dla.mOpenAreaCount << "\n";
    ss << " - OpenAreaRadius: " << mParams.dla.mOpenAreaRadius << "\n";
    break;
  case CaveType::BSP_TECTONIC:
    ss << " BSP_TECTONIC\n";
    ss << " - BspDepth: " << mParams.bspTectonic.mBspDepth << "\n";
    ss << " - MinRoomSize: " << mParams.bspTectonic.mMinRoomSize << "\n";
    ss << " - RoomPadding: " << mParams.bspTectonic.mRoomPadding << "\n";
    break;
  case CaveType::EMPTY:
    ss << " EMPTY\n";
  }
  return ss.str();
}

TileMap Cave::generateEmpty(int width, int height) {
  TileMap tileMap(height, std::vector<int>(width, FLOOR));
  int xgap = width / 8;
  int ygap = height / 8;
  for (int cy = 0; cy < height; cy += ygap) {
    for (int cx = 0; cx < width; ++cx) {
      tileMap[cy][cx] = WALL;
    }
  }
  for (int cx = 0; cx < width; cx += xgap) {
    for (int cy = 0; cy < height; ++cy) {
      tileMap[cy][cx] = WALL;
    }
  }

  return tileMap;
}

} // namespace Cave
