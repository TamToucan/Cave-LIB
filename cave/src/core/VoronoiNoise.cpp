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
// Worley Noise
// ------------------------------------------------------------

struct Vector2f {
  float x, y;
};

// A 2D hash for Worley Noise feature points
inline Vector2f hash22(Vector2f p) {
  Vector2f q = {std::fmod(p.x * 127.1f + p.y * 311.7f, 289.0f),
                std::fmod(p.x * 269.5f + p.y * 183.3f, 289.0f)};
  return {std::abs(std::fmod(std::sin(q.x) * 43758.5453f, 1.0f)),
          std::abs(std::fmod(std::sin(q.y) * 43758.5453f, 1.0f))};
}

// Compute Worley ridge metric (d2 - d1)
float VoronoiNoise::worleyMetric(float px, float py, uint32_t seed) {
  Vector2f p = {px, py};
  Vector2f pi = {std::floor(p.x), std::floor(p.y)};
  Vector2f pf = {p.x - pi.x, p.y - pi.y};

  float d1 = std::numeric_limits<float>::max();
  float d2 = std::numeric_limits<float>::max();

  // Incorporate seed loosely by translating the grid
  float sox = (seed & 0xFF) / 255.0f * 10.0f;
  float soy = ((seed >> 8) & 0xFF) / 255.0f * 10.0f;

  for (int y = -1; y <= 1; ++y) {
    for (int x = -1; x <= 1; ++x) {
      Vector2f neighbor = {static_cast<float>(x), static_cast<float>(y)};
      Vector2f gridPt = {pi.x + neighbor.x + sox, pi.y + neighbor.y + soy};
      Vector2f offset = hash22(gridPt);

      Vector2f diff = {neighbor.x + offset.x - pf.x,
                       neighbor.y + offset.y - pf.y};
      float d = diff.x * diff.x + diff.y * diff.y;

      if (d < d1) {
        d2 = d1;
        d1 = d;
      } else if (d < d2) {
        d2 = d;
      }
    }
  }

  // Multiply by a factor so it behaves similarly numerically to the old metric
  // The distance between adjacent cells in unit-cell Worley maxes around 1.0.
  // We want the resulting (d2-d1) to range [0, 1] roughly.
  return (std::sqrt(d2) - std::sqrt(d1));
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

  // A magic multiplier to keep the visual scale roughly similar to before
  // when scale=0.02. Worley cells are 1x1 in the noise space.
  // Old logic had ~5x6 cells across 96x64 => ~1 cell per 16 pixels.
  // So scale=0.02 * 16 ~ 0.32. We'll bake this factor in so 0.02 feels similar.
  const float worleyScaleMult = 3.0f;

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

      // Evaluate continuous Worley noise using the warped, scaled coordinates
      float wnx = wx * vp.mNoiseScale * worleyScaleMult;
      float wny = wy * vp.mNoiseScale * worleyScaleMult;

      float v = worleyMetric(wnx, wny, seed + 100);

      // Worley v is naturally in [0, 1] rough range, clamp just in case
      v = std::min(v, 1.0f);
      float combined = base + vorWeight * v;

      g[y][x] = (combined > vp.mThreshold) ? FLOOR : WALL;
    }
  }

  smoothGrid(g);

  return g;
}

} // namespace Cave