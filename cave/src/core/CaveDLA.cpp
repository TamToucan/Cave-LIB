#include "CaveDLA.h"
#include "RandUniversal.h"
#include "TileTypes.h"
#include <vector>

namespace Cave {

TileMap CaveDLA::generate(int width, int height,
                          const GenerationParams &params) {
  TileMap map(height, std::vector<int>(width, WALL));

  int centerX = width / 2;
  int centerY = height / 2;
  map[centerY][centerX] = FLOOR;

  RNG::RandUniversal simple(params.seed);

  const int dx[] = {0, 0, -1, 1};
  const int dy[] = {-1, 1, 0, 0};

  for (int p = 0; p < params.dla.mParticleCount; ++p) {
    int px, py;
    // Spawn at edge
    if (simple.getFloat() < 0.5f) {
      px = 1 + (int)(simple.getFloat() * (width - 2));
      py = (simple.getFloat() < 0.5f) ? 1 : height - 2;
    } else {
      px = (simple.getFloat() < 0.5f) ? 1 : width - 2;
      py = 1 + (int)(simple.getFloat() * (height - 2));
    }

    // Random walk
    for (int step = 0; step < 10000; ++step) { // limit walk steps
      // Check neighbors
      bool touchingFloor = false;
      for (int i = 0; i < 4; ++i) {
        int nx = px + dx[i];
        int ny = py + dy[i];
        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
          if (map[ny][nx] == FLOOR) {
            touchingFloor = true;
            break;
          }
        }
      }

      if (touchingFloor) {
        map[py][px] = FLOOR;
        break; // Particle crystallized
      }

      // Move particle
      int dir = (int)(simple.getFloat() * 4.0f);
      if (dir >= 4)
        dir = 3;
      int nx = px + dx[dir];
      int ny = py + dy[dir];

      if (nx >= 1 && nx < width - 1 && ny >= 1 && ny < height - 1) {
        px = nx;
        py = ny;
      }
    }
  }

  // Secondary pass: Carve open areas
  if (params.dla.mOpenAreaCount > 0) {
    // Collect all current floor tile coords
    std::vector<std::pair<int, int>> floorTiles;
    for (int y = 1; y < height - 1; ++y) {
      for (int x = 1; x < width - 1; ++x) {
        if (map[y][x] == FLOOR) {
          floorTiles.push_back({x, y});
        }
      }
    }

    if (!floorTiles.empty()) {
      int radius = params.dla.mOpenAreaRadius;
      int r2 = radius * radius;

      for (int i = 0; i < params.dla.mOpenAreaCount; ++i) {
        int idx = (int)(simple.getFloat() * floorTiles.size());
        if (idx >= floorTiles.size())
          idx = floorTiles.size() - 1;
        auto [cx, cy] = floorTiles[idx];

        for (int y = -radius; y <= radius; ++y) {
          for (int x = -radius; x <= radius; ++x) {
            if (x * x + y * y <= r2) {
              int px = cx + x;
              int py = cy + y;
              if (px >= 1 && px < width - 1 && py >= 1 && py < height - 1) {
                map[py][px] = FLOOR;
              }
            }
          }
        }
      }
    }
  }

  return map;
}

} // namespace Cave
