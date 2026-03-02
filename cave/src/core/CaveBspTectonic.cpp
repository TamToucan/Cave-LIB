#include "CaveBspTectonic.h"

#include <vector>

#include "PerlinNoise.h"
#include "RandSimple.h"
#include "RogueCave.hpp"
#include "SimplexNoise.h"

namespace Cave {

struct BspRect {
  int x, y, w, h;
};

static void splitNode(std::vector<BspRect> &leaves, BspRect node, int depth,
                      int maxDepth, int minSize, RNG::RandSimple &rng) {
  if (depth >= maxDepth) {
    leaves.push_back(node);
    return;
  }

  // Can we split?
  bool canSplitH = node.h >= minSize * 2;
  bool canSplitV = node.w >= minSize * 2;

  if (!canSplitH && !canSplitV) {
    leaves.push_back(node);
    return;
  }

  bool splitH = false;
  if (canSplitH && canSplitV) {
    splitH = rng.getFloat() > 0.5f;
  } else if (canSplitH) {
    splitH = true;
  }

  if (splitH) {
    // Split horizontally (cut across y axis)
    int splitPoint = rng.getInt(minSize, node.h - minSize);
    BspRect top = {node.x, node.y, node.w, splitPoint};
    BspRect bottom = {node.x, node.y + splitPoint, node.w, node.h - splitPoint};
    splitNode(leaves, top, depth + 1, maxDepth, minSize, rng);
    splitNode(leaves, bottom, depth + 1, maxDepth, minSize, rng);
  } else {
    // Split vertically (cut across x axis)
    int splitPoint = rng.getInt(minSize, node.w - minSize);
    BspRect left = {node.x, node.y, splitPoint, node.h};
    BspRect right = {node.x + splitPoint, node.y, node.w - splitPoint, node.h};
    splitNode(leaves, left, depth + 1, maxDepth, minSize, rng);
    splitNode(leaves, right, depth + 1, maxDepth, minSize, rng);
  }
}

TileMap CaveBspTectonic::generate(int width, int height,
                                  const GenerationParams &params) {
  TileMap tileMap(height, std::vector<int>(width, WALL));

  RNG::RandSimple rng(params.seed);
  const BspTectonicParams &bspParams = params.bspTectonic;

  std::vector<BspRect> leaves;
  BspRect root = {0, 0, width, height};
  splitNode(leaves, root, 0, bspParams.mBspDepth, bspParams.mMinRoomSize, rng);

  for (const BspRect &leaf : leaves) {
    // Apply padding
    int rx = leaf.x + bspParams.mRoomPadding;
    int ry = leaf.y + bspParams.mRoomPadding;
    int rw = leaf.w - bspParams.mRoomPadding * 2;
    int rh = leaf.h - bspParams.mRoomPadding * 2;

    if (rw <= 0 || rh <= 0)
      continue;

    // Generate CA for this room
    PCG::RogueCave roomCA(rw, rh);
    std::vector<std::vector<int>> &gridIn = roomCA.getGrid();

    const double W = rw - 1 + bspParams.mCaParams.mAmp;
    const double H = rh - 1 + bspParams.mCaParams.mAmp;
    // Perlin logic matches standard CA initialization
    double (*pf)(double, double, int) =
        bspParams.mCaParams.mPerlin ? &Algo::getSNoise2 : &Algo::getNoise2;

    // Use a slightly offset seed for each node to ensure variance when not
    // using perlin
    int offsetSeed = params.seed + (leaf.x * 13) + (leaf.y * 7);
    RNG::RandSimple localRng(offsetSeed);

    for (int y = 0; y < rh; ++y) {
      for (int x = 0; x < rw; ++x) {
        // Ensure the borders are walls so the CA doesn't spill open
        if (x == 0 || x == rw - 1 || y == 0 || y == rh - 1) {
          gridIn[y][x] = PCG::RogueCave::TILE_WALL;
        } else {
          double nx = x / W * bspParams.mCaParams.mFreq;
          double ny = y / H * bspParams.mCaParams.mFreq;

          double noiseVal = 0.0;
          if (bspParams.mCaParams.mPerlin) {
            noiseVal = (*pf)(nx, ny, bspParams.mCaParams.mOctaves);
          } else {
            noiseVal = localRng.getFloat() - bspParams.mCaParams.mWallChance;
          }

          gridIn[y][x] = (noiseVal < 0) ? PCG::RogueCave::TILE_WALL
                                        : PCG::RogueCave::TILE_FLOOR;
        }
      }
    }

    // Run generations
    for (const auto &gen : bspParams.mCaParams.mGenerations) {
      Util::IntRange b3(gen.b3_min, gen.b3_max);
      Util::IntRange b5(gen.b5_min, gen.b5_max);
      Util::IntRange s3(gen.s3_min, gen.s3_max);
      Util::IntRange s5(gen.s5_min, gen.s5_max);
      roomCA.addGeneration(b3, b5, s3, s5, gen.reps);
    }

    std::vector<std::vector<int>> &gridOut = roomCA.generate();

    // Copy back to tilemap
    for (int y = 0; y < rh; ++y) {
      for (int x = 0; x < rw; ++x) {
        if (gridOut[y][x] == PCG::RogueCave::TILE_FLOOR) {
          if (rx + x >= 0 && rx + x < width && ry + y >= 0 && ry + y < height) {
            tileMap[ry + y][rx + x] = FLOOR;
          }
        }
      }
    }
  }

  return tileMap;
}

} // namespace Cave
