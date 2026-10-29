#include "CaveZones.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "CaveBspTectonic.h"
#include "CaveCellular.h"
#include "CaveCity.h"
#include "CaveHeightmap.h"
#include "CaveMine.h"
#include "CaveVeins.h"
#include "RandUniversal.h"
#include "VoronoiNoise.h"

namespace Cave {

namespace {

constexpr int kMinSlot = 24;        ///< least slot side, each way (§2 step 1)
constexpr int kSlotSeedStep = 7919; ///< slot i seed = seed + step * (i + 1)
constexpr int kJoinTries = 8;       ///< places tried per join

/// Half-open rectangle [x0, x1) x [y0, y1).
struct Rect {
  int x0;
  int y0;
  int x1;
  int y1;
  int w() const { return x1 - x0; }
  int h() const { return y1 - y0; }
};

struct Slot {
  Rect r;        ///< slot area (tiles the map with the others)
  int typeIndex; ///< index into CaveZones::kSlotTypes
};

/// Split [0, len) into n parts at the even places, each moved by jitter.
std::vector<int> splitLine(int len, int n, float jitter,
                           RNG::RandUniversal &rng) {
  std::vector<int> cuts = {0};
  const int j = static_cast<int>(std::lround(jitter * (len / n)));
  for (int i = 1; i < n; ++i) {
    cuts.push_back(i * len / n + rng.getInt(-j, j));
  }
  cuts.push_back(len);
  return cuts;
}

bool fits(const std::vector<Slot> &slots) {
  for (const Slot &s : slots) {
    if (s.r.w() < kMinSlot || s.r.h() < kMinSlot) {
      return false;
    }
  }
  return true;
}

/// Slots for one layout; empty if a slot would be under kMinSlot.
std::vector<Slot> layoutSlots(int layout, int w, int h, const ZonesParams &zp,
                              RNG::RandUniversal &rng) {
  const bool wide = w >= h;
  const int len = wide ? w : h;
  const auto band = [&](int n) {
    std::vector<Slot> out;
    const std::vector<int> cuts = splitLine(len, n, zp.mSplitJitter, rng);
    for (int i = 0; i < n; ++i) {
      const Rect r = wide ? Rect{cuts[i], 0, cuts[i + 1], h}
                          : Rect{0, cuts[i], w, cuts[i + 1]};
      out.push_back({r, zp.mSlotType[i]});
    }
    return out;
  };
  std::vector<Slot> slots;
  switch (layout) {
  case CaveZones::GAUNTLET:
    slots = band(3);
    break;
  case CaveZones::HARD_CENTRE: {
    const int cw = static_cast<int>(std::lround(zp.mCentreSize * w));
    const int ch = static_cast<int>(std::lround(zp.mCentreSize * h));
    const int x0 = (w - cw) / 2, y0 = (h - ch) / 2;
    const int ring = zp.mSlotType[1];
    slots = {{{x0, y0, x0 + cw, y0 + ch}, zp.mSlotType[0]},
             {{0, 0, w, y0}, ring},
             {{0, y0 + ch, w, h}, ring},
             {{0, y0, x0, y0 + ch}, ring},
             {{x0 + cw, y0, w, y0 + ch}, ring}};
    break;
  }
  case CaveZones::QUAD: {
    const std::vector<int> xs = splitLine(w, 2, zp.mSplitJitter, rng);
    const std::vector<int> ys = splitLine(h, 2, zp.mSplitJitter, rng);
    for (int i = 0; i < 4; ++i) {
      const int cx = i % 2, cy = i / 2;
      slots.push_back({{xs[cx], ys[cy], xs[cx + 1], ys[cy + 1]}, zp.mSlotType[i]});
    }
    break;
  }
  default:
    slots = band(2);
    break;
  }
  return fits(slots) ? slots : std::vector<Slot>{};
}

TileMap runSlot(CaveType type, int w, int h, const GenerationParams &p) {
  switch (type) {
  case CaveType::VORONOI:
    return VoronoiNoise::generate(w, h, p);
  case CaveType::HEIGHTMAP:
    return CaveHeightmap::generate(w, h, p);
  case CaveType::BSP_TECTONIC:
    return CaveBspTectonic::generate(w, h, p);
  case CaveType::MINE:
    return CaveMine::generate(w, h, p);
  case CaveType::CITY:
    return CaveCity::generate(w, h, p);
  case CaveType::VEINS:
    return CaveVeins::generate(w, h, p);
  default:
    return CaveCellular::generate(w, h, p);
  }
}

/// Two slots side by side: the border line and the span both share.
struct Border {
  bool vertical; ///< true = the line is x = at
  int at;
  int lo; ///< shared span along the line, [lo, hi)
  int hi;
  int a;  ///< slot before the line
  int b;  ///< slot after the line
};

std::vector<Border> findBorders(const std::vector<Slot> &slots) {
  std::vector<Border> out;
  for (int a = 0; a < static_cast<int>(slots.size()); ++a) {
    for (int b = 0; b < static_cast<int>(slots.size()); ++b) {
      const Rect &ra = slots[static_cast<size_t>(a)].r;
      const Rect &rb = slots[static_cast<size_t>(b)].r;
      if (ra.x1 == rb.x0) {
        const int lo = std::max(ra.y0, rb.y0), hi = std::min(ra.y1, rb.y1);
        if (hi > lo) {
          out.push_back({true, ra.x1, lo, hi, a, b});
        }
      }
      if (ra.y1 == rb.y0) {
        const int lo = std::max(ra.x0, rb.x0), hi = std::min(ra.x1, rb.x1);
        if (hi > lo) {
          out.push_back({false, ra.y1, lo, hi, a, b});
        }
      }
    }
  }
  return out;
}

} // namespace

TileMap CaveZones::generate(int width, int height,
                            const GenerationParams &params) {
  const ZonesParams &zp = params.zones;
  RNG::RandUniversal rng(params.seed);
  TileMap map(static_cast<size_t>(height),
              std::vector<int>(static_cast<size_t>(width), WALL));

  // 1-2. Fit the layout, falling back; one slot if even LAIR does not fit.
  std::vector<Slot> slots;
  for (int layout = zp.mLayout; slots.empty();) {
    slots = layoutSlots(layout, width, height, zp, rng);
    if (!slots.empty()) {
      break;
    }
    if (layout == HARD_CENTRE) {
      layout = QUAD;
    } else if (layout != LAIR) {
      layout = LAIR;
    } else {
      slots = {{{0, 0, width, height}, zp.mSlotType[0]}};
    }
  }

  // Borders, and each slot's inner area: inset on every side that is not the
  // map edge, so a border band of borderWidth sits on each split line.
  const int bw = std::max(1, zp.mBorderWidth);
  const int before = bw / 2;
  const int after = bw - before;
  const std::vector<Border> borders = findBorders(slots);
  std::vector<Rect> inner;
  for (const Slot &s : slots) {
    Rect r = s.r;
    r.x0 += s.r.x0 > 0 ? after : 0;
    r.y0 += s.r.y0 > 0 ? after : 0;
    r.x1 -= s.r.x1 < width ? before : 0;
    r.y1 -= s.r.y1 < height ? before : 0;
    inner.push_back(r);
  }

  // 3. Fill slots.
  for (size_t i = 0; i < slots.size(); ++i) {
    const Rect &r = inner[i];
    if (r.w() <= 0 || r.h() <= 0) {
      continue;
    }
    GenerationParams sp = params;
    const int idx = std::clamp(slots[i].typeIndex, 0, kSlotTypeCount - 1);
    sp.mCaveType = kSlotTypes[idx];
    sp.seed = params.seed + kSlotSeedStep * static_cast<int>(i + 1);
    const TileMap tiles = runSlot(sp.mCaveType, r.w(), r.h(), sp);
    for (int y = 0; y < r.h(); ++y) {
      for (int x = 0; x < r.w(); ++x) {
        map[static_cast<size_t>(r.y0 + y)][static_cast<size_t>(r.x0 + x)] =
            tiles[static_cast<size_t>(y)][static_cast<size_t>(x)];
      }
    }
  }

  // 4. Joins: walk across the border both ways to the first floor.
  const int jw = std::max(1, zp.mJoinWidth);
  const auto floorAt = [&](int along, int across, bool vertical) {
    const int x = vertical ? across : along, y = vertical ? along : across;
    return map[static_cast<size_t>(y)][static_cast<size_t>(x)] == FLOOR;
  };
  for (const Border &bd : borders) {
    const Rect &ia = inner[static_cast<size_t>(bd.a)];
    const Rect &ib = inner[static_cast<size_t>(bd.b)];
    // Span both inner areas cover, so a join never runs along a border.
    const int lo = std::max(bd.lo, bd.vertical ? std::max(ia.y0, ib.y0)
                                               : std::max(ia.x0, ib.x0));
    const int hi = std::min(bd.hi, bd.vertical ? std::min(ia.y1, ib.y1)
                                               : std::min(ia.x1, ib.x1));
    if (hi - lo < jw) {
      continue;
    }
    const int edgeA = bd.vertical ? ia.x1 : ia.y1; // first cell past slot a
    const int edgeB = bd.vertical ? ib.x0 : ib.y0; // first cell of slot b
    const int depthA = (bd.vertical ? ia.w() : ia.h()) / 2;
    const int depthB = (bd.vertical ? ib.w() : ib.h()) / 2;
    for (int j = 0; j < zp.mJoinsPerBorder; ++j) {
      for (int t = 0; t < kJoinTries; ++t) {
        const int along = rng.getInt(lo, hi - jw);
        int fa = -1, fb = -1;
        for (int d = 1; d <= depthA && fa < 0; ++d) {
          if (floorAt(along, edgeA - d, bd.vertical)) {
            fa = edgeA - d;
          }
        }
        for (int d = 0; d < depthB && fb < 0; ++d) {
          if (floorAt(along, edgeB + d, bd.vertical)) {
            fb = edgeB + d;
          }
        }
        if (fa < 0 || fb < 0) {
          continue;
        }
        for (int k = 0; k < jw; ++k) {
          for (int c = fa; c <= fb; ++c) {
            const int x = bd.vertical ? c : along + k;
            const int y = bd.vertical ? along + k : c;
            map[static_cast<size_t>(y)][static_cast<size_t>(x)] = FLOOR;
          }
        }
        break;
      }
    }
  }
  return map;
}

} // namespace Cave
