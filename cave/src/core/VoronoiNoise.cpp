#include "VoronoiNoise.h"
#include "RandUniversal.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
// ------------------------------------------------------------
// Basic Hash + Value Noise
// ------------------------------------------------------------

inline uint32_t hash(uint32_t x) {
  x ^= x >> 16;
  x *= 0x7feb352d;
  x ^= x >> 15;
  x *= 0x846ca68b;
  x ^= x >> 16;
  return x;
}

inline float rand01(int x, int y, uint32_t seed) {
  uint32_t h = hash(x * 374761393u + y * 668265263u + seed);
  return (h & 0xFFFFFF) / float(0xFFFFFF);
}

inline float lerp(float a, float b, float t) { return a + t * (b - a); }

inline float smooth(float t) { return t * t * (3.f - 2.f * t); }

float valueNoise(float x, float y, uint32_t seed) {
  int x0 = static_cast<int>(std::floor(x));
  int y0 = static_cast<int>(std::floor(y));
  int x1 = x0 + 1;
  int y1 = y0 + 1;

  float sx = smooth(x - x0);
  float sy = smooth(y - y0);

  float n00 = rand01(x0, y0, seed);
  float n10 = rand01(x1, y0, seed);
  float n01 = rand01(x0, y1, seed);
  float n11 = rand01(x1, y1, seed);

  float ix0 = lerp(n00, n10, sx);
  float ix1 = lerp(n01, n11, sx);

  return lerp(ix0, ix1, sy);
}

} // namespace

namespace Cave {

// ------------------------------------------------------------
// Voronoi Seeds (stratified placement)
// ------------------------------------------------------------

std::vector<Vector2i> VoronoiNoise::generateSeeds(int W, int H, int cellsX,
                                                  int cellsY, uint32_t seed) {
  std::vector<Vector2i> seeds;
  RNG::RandUniversal rng(seed);

  float cellW = W / float(cellsX);
  float cellH = H / float(cellsY);

  for (int y = 0; y < cellsY; ++y) {
    for (int x = 0; x < cellsX; ++x) {
      int sx = int((x + rng.getFloat()) * cellW);
      int sy = int((y + rng.getFloat()) * cellH);
      seeds.push_back({sx, sy});
    }
  }
  return seeds;
}

// Compute Voronoi ridge metric: d2 - d1
float VoronoiNoise::voronoiMetric(int x, int y,
                                  const std::vector<Vector2i> &seeds) {
  float d1 = std::numeric_limits<float>::max();
  float d2 = std::numeric_limits<float>::max();

  for (const auto &s : seeds) {
    float dx = float(x - s.x);
    float dy = float(y - s.y);
    float d = dx * dx + dy * dy;

    if (d < d1) {
      d2 = d1;
      d1 = d;
    } else if (d < d2) {
      d2 = d;
    }
  }
  return std::sqrt(d2) - std::sqrt(d1);
}

// ------------------------------------------------------------
// Smoothing pass
// ------------------------------------------------------------

void VoronoiNoise::smoothGrid(TileMap &g) {
  int H = static_cast<int>(g.size());
  int W = static_cast<int>(g[0].size());
  TileMap copy = g;

  for (int y = 1; y < H - 1; ++y) {
    for (int x = 1; x < W - 1; ++x) {
      int wallCount = 0;
      for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
          if (copy[y + dy][x + dx] == WALL)
            wallCount++;

      if (wallCount >= 5)
        g[y][x] = WALL;
      else
        g[y][x] = FLOOR;
    }
  }
}

// ------------------------------------------------------------
// Main Cave Generator
// ------------------------------------------------------------

TileMap VoronoiNoise::generate(int W, int H, const GenerationParams &params) {
  TileMap g(H, std::vector<int>(W, WALL));
  uint32_t seed = static_cast<uint32_t>(params.seed);
  const VoronoiParams &vp = params.voronoi;
  const float vorWeight =
      (vp.mInvertVoronoi) ? vp.mVoronoiWeight : -vp.mVoronoiWeight;

  auto seeds = generateSeeds(W, H, 6, 5, seed + 100);

  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      // Domain warp
      float wx =
          x + vp.mWarpStrength *
                  valueNoise(x * vp.mNoiseScale, y * vp.mNoiseScale, seed + 1);
      float wy = y + vp.mWarpStrength * valueNoise((x + 200) * vp.mNoiseScale,
                                                   (y + 200) * vp.mNoiseScale,
                                                   seed + 2);

      float base = valueNoise(wx * vp.mNoiseScale, wy * vp.mNoiseScale, seed);

      float v = voronoiMetric(x, y, seeds);
      v = std::min(v / 40.f, 1.f);
      float combined = base + vorWeight * v;

      g[y][x] = (combined > vp.mThreshold) ? FLOOR : WALL;
    }
  }

  smoothGrid(g);

  return g;
}

} // namespace Cave