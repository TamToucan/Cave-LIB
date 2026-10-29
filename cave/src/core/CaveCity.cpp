#include "CaveCity.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "RandUniversal.h"

namespace Cave {

namespace {

/// Least plaza side: room_detection.md C-05 seeds a room only at wall
/// distance >= 3, so each ward holds a hall this big.
constexpr int kMinHall = 7;
/// Gate width: under room_detection.md C-06's 3-tile gate, so the room
/// finder keeps each ward a separate room. Also the widest door.
constexpr int kGateWidth = 2;
/// Least block side that can be hollow (shell + 3x3 floor).
constexpr int kMinHollowSide = 5;
constexpr double kTriesPerTiles = 1000.0; ///< blockRate unit

/// Half-open span [lo, hi) of ward cells along one axis.
struct Span {
  int lo;
  int hi;
};

/// Split [0, len) into wards with a wall of `wall` tiles between; every ward
/// keeps at least kMinHall cells (cave_gen_city.md §2 step 1).
std::vector<Span> splitAxis(int len, const CityParams &cp,
                            RNG::RandUniversal &rng) {
  const int n = std::max(1, len / std::max(1, cp.mWardSize));
  const int wall = std::max(1, cp.mWardWall);
  const int j =
      std::max(0, std::min(cp.mWardJitter, (len / n - wall - kMinHall) / 2));
  std::vector<Span> spans;
  int start = 0;
  for (int i = 1; i < n; ++i) {
    const int w0 = i * len / n - wall / 2 + rng.getInt(-j, j);
    spans.push_back({start, w0});
    start = w0 + wall;
  }
  spans.push_back({start, len});
  return spans;
}

struct City {
  int w;
  int h;
  TileMap cells;
  std::vector<std::vector<char>> taken; ///< plaza and block cells
  City(int width, int height)
      : w(width), h(height), cells(height, std::vector<int>(width, FLOOR)),
        taken(height, std::vector<char>(width, 0)) {}
  void fill(int x0, int y0, int x1, int y1, int tile) {
    for (int y = std::max(y0, 0); y < std::min(y1, h); ++y) {
      for (int x = std::max(x0, 0); x < std::min(x1, w); ++x) {
        cells[y][x] = tile;
      }
    }
  }
  void mark(int x0, int y0, int x1, int y1) {
    for (int y = y0; y < y1; ++y) {
      for (int x = x0; x < x1; ++x) {
        taken[y][x] = 1;
      }
    }
  }
  /// True if [x0, x1) x [y0, y1) is inside the ward and holds no wall and
  /// no taken cell.
  bool clear(int x0, int y0, int x1, int y1, const Span &wx,
             const Span &wy) const {
    if (x0 < wx.lo || y0 < wy.lo || x1 > wx.hi || y1 > wy.hi) {
      return false;
    }
    for (int y = y0; y < y1; ++y) {
      for (int x = x0; x < x1; ++x) {
        if (cells[y][x] == WALL || taken[y][x]) {
          return false;
        }
      }
    }
    return true;
  }
};

/// Hollow out an accepted block and cut its doors (§2 step 5).
void hollow(City &c, int bx, int by, int bw, int bh, const CityParams &cp,
            RNG::RandUniversal &rng) {
  c.fill(bx + 1, by + 1, bx + bw - 1, by + bh - 1, FLOOR);
  const int doors = rng.getInt(1, std::max(1, cp.mDoorsMax));
  const int dw = std::clamp(cp.mDoorWidth, 1, kGateWidth);
  std::vector<int> unused = {0, 1, 2, 3}; // top, bottom, left, right
  for (int d = 0; d < doors; ++d) {
    int side;
    if (!unused.empty()) {
      const int k = rng.getInt(0, static_cast<int>(unused.size()) - 1);
      side = unused[static_cast<size_t>(k)];
      unused.erase(unused.begin() + k);
    } else {
      side = rng.getInt(0, 3);
    }
    const int len = side < 2 ? bw : bh;
    // Not touching a corner: door cells in [1, len - 1).
    const int at = rng.getInt(1, len - 1 - dw);
    switch (side) {
    case 0:
      c.fill(bx + at, by, bx + at + dw, by + 1, FLOOR);
      break;
    case 1:
      c.fill(bx + at, by + bh - 1, bx + at + dw, by + bh, FLOOR);
      break;
    case 2:
      c.fill(bx, by + at, bx + 1, by + at + dw, FLOOR);
      break;
    default:
      c.fill(bx + bw - 1, by + at, bx + bw, by + at + dw, FLOOR);
      break;
    }
  }
}

/// An accepted block, [x, x + w) x [y, y + h).
struct Block {
  int x;
  int y;
  int w;
  int h;
};

/// One block try in a ward (§2 step 4). Rejected tries are not retried.
void tryBlock(City &c, int bx, int by, int bw, int bh, int s, const Span &wx,
              const Span &wy, std::vector<Block> &blocks) {
  if (!c.clear(bx - s, by - s, bx + bw + s, by + bh + s, wx, wy)) {
    return;
  }
  c.fill(bx, by, bx + bw, by + bh, WALL);
  c.mark(bx, by, bx + bw, by + bh);
  blocks.push_back({bx, by, bw, bh});
}

/// Grow each block one side at a time while its street ring stays clear and
/// its side stays <= cap, until nothing grows. No RNG.
void growBlocks(City &c, std::vector<Block> &blocks, int s, int cap,
                const Span &wx, const Span &wy) {
  bool grew = true;
  while (grew) {
    grew = false;
    for (Block &b : blocks) {
      if (b.w < cap &&
          c.clear(b.x + b.w + s, b.y - s, b.x + b.w + s + 1, b.y + b.h + s,
                  wx, wy)) {
        c.fill(b.x + b.w, b.y, b.x + b.w + 1, b.y + b.h, WALL);
        c.mark(b.x + b.w, b.y, b.x + b.w + 1, b.y + b.h);
        ++b.w;
        grew = true;
      }
      if (b.h < cap &&
          c.clear(b.x - s, b.y + b.h + s, b.x + b.w + s, b.y + b.h + s + 1,
                  wx, wy)) {
        c.fill(b.x, b.y + b.h, b.x + b.w, b.y + b.h + 1, WALL);
        c.mark(b.x, b.y + b.h, b.x + b.w, b.y + b.h + 1);
        ++b.h;
        grew = true;
      }
      if (b.w < cap &&
          c.clear(b.x - s - 1, b.y - s, b.x - s, b.y + b.h + s, wx, wy)) {
        --b.x;
        ++b.w;
        c.fill(b.x, b.y, b.x + 1, b.y + b.h, WALL);
        c.mark(b.x, b.y, b.x + 1, b.y + b.h);
        grew = true;
      }
      if (b.h < cap &&
          c.clear(b.x - s, b.y - s - 1, b.x + b.w + s, b.y - s, wx, wy)) {
        --b.y;
        ++b.h;
        c.fill(b.x, b.y, b.x + b.w, b.y + 1, WALL);
        c.mark(b.x, b.y, b.x + b.w, b.y + 1);
        grew = true;
      }
    }
  }
}

} // namespace

TileMap CaveCity::generate(int width, int height,
                           const GenerationParams &params) {
  const CityParams &cp = params.city;
  RNG::RandUniversal rng(params.seed);
  City c(width, height);

  // 1. Ward grid: split walls run the full map.
  const std::vector<Span> xs = splitAxis(width, cp, rng);
  const std::vector<Span> ys = splitAxis(height, cp, rng);
  for (size_t k = 0; k + 1 < xs.size(); ++k) {
    c.fill(xs[k].hi, 0, xs[k + 1].lo, height, WALL);
  }
  for (size_t k = 0; k + 1 < ys.size(); ++k) {
    c.fill(0, ys[k].hi, width, ys[k + 1].lo, WALL);
  }

  // 2. Gates: one per pair of wards side by side.
  for (size_t k = 0; k + 1 < xs.size(); ++k) {
    for (const Span &wy : ys) {
      const int y = rng.getInt(wy.lo, wy.hi - kGateWidth);
      c.fill(xs[k].hi, y, xs[k + 1].lo, y + kGateWidth, FLOOR);
    }
  }
  for (size_t k = 0; k + 1 < ys.size(); ++k) {
    for (const Span &wx : xs) {
      const int x = rng.getInt(wx.lo, wx.hi - kGateWidth);
      c.fill(x, ys[k].hi, x + kGateWidth, ys[k + 1].lo, FLOOR);
    }
  }

  const int plazaMax = std::max(kMinHall, cp.mPlazaSize);
  const int s = std::max(1, cp.mStreetWidth);
  const int lo = std::max(1, std::min(cp.mBlockMin, cp.mBlockMax));
  const int hi = std::max(lo, cp.mBlockMax);
  for (const Span &wy : ys) {
    for (const Span &wx : xs) {
      // 3. Plaza: the ward's hall.
      const int pw = std::min(rng.getInt(kMinHall, plazaMax), wx.hi - wx.lo);
      const int ph = std::min(rng.getInt(kMinHall, plazaMax), wy.hi - wy.lo);
      const int px = rng.getInt(wx.lo, wx.hi - pw);
      const int py = rng.getInt(wy.lo, wy.hi - ph);
      c.mark(px, py, px + pw, py + ph);

      // 4. Blocks, then grow them to fill the gaps.
      std::vector<Block> blocks;
      if (cp.mGridMode) {
        const int cell = rng.getInt(lo, hi); // one lattice size per ward
        for (int gy = wy.lo + s; gy < wy.hi; gy += cell + s) {
          for (int gx = wx.lo + s; gx < wx.hi; gx += cell + s) {
            const int bx = gx + rng.getInt(-cp.mGridJitter, cp.mGridJitter);
            const int by = gy + rng.getInt(-cp.mGridJitter, cp.mGridJitter);
            tryBlock(c, bx, by, cell, cell, s, wx, wy, blocks);
          }
        }
      } else {
        const int area = (wx.hi - wx.lo) * (wy.hi - wy.lo);
        const long tries =
            std::lround(cp.mBlockRate * area / kTriesPerTiles);
        for (long t = 0; t < tries; ++t) {
          const int bw = rng.getInt(lo, hi);
          const int bh = rng.getInt(lo, hi);
          // Top-left only where block + street ring fit in the ward.
          if (wx.hi - wx.lo < bw + 2 * s || wy.hi - wy.lo < bh + 2 * s) {
            continue;
          }
          const int bx = rng.getInt(wx.lo + s, wx.hi - s - bw);
          const int by = rng.getInt(wy.lo + s, wy.hi - s - bh);
          tryBlock(c, bx, by, bw, bh, s, wx, wy, blocks);
        }
      }
      growBlocks(c, blocks, s, hi, wx, wy);

      // 6. Hollow buildings.
      for (const Block &b : blocks) {
        if (b.w >= kMinHollowSide && b.h >= kMinHollowSide &&
            rng.getFloat() < cp.mHollowShare) {
          hollow(c, b.x, b.y, b.w, b.h, cp, rng);
        }
      }
    }
  }
  return c.cells;
}

} // namespace Cave
