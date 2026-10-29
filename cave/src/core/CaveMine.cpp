#include "CaveMine.h"

#include <algorithm>
#include <vector>

#include "RandUniversal.h"

namespace Cave {

namespace {

constexpr int kMinEntryWidth = 2; ///< no designed gap under 2 tiles (GC-05)
constexpr int kMinPanelRows = 2;  ///< least cross-cut rows in a panel
constexpr int kMinStubSide = 2;   ///< fixUp removes 1-tile wall specks
/// Least hall side: room_detection.md C-05 seeds a room only at wall
/// distance >= 3, so each panel and the main hold a hall this big.
constexpr int kMinHall = 7;
/// Opening width: under room_detection.md C-06's 3-tile gate, so the room
/// finder keeps each panel and the main as separate rooms.
constexpr int kGateWidth = 2;

enum class PanelType { PILLAR, ROBBED, LONGWALL, UNMINED };

struct Panel {
  int x0 = 0;       ///< span along the main, [x0, x1)
  int x1 = 0;
  int colStart = 0; ///< x of the first entry column
  int cols = 0;     ///< entry columns
  int extent = 0;   ///< width covered by the columns
  int rows = 0;     ///< cross-cut rows; depth = rows * pitchX
  PanelType type = PanelType::PILLAR;
};

/// Working grid with the long axis along x. fill() clips to the grid.
struct Grid {
  int w;
  int h;
  TileMap cells;
  Grid(int width, int height)
      : w(width), h(height), cells(height, std::vector<int>(width, WALL)) {}
  void fill(int x0, int y0, int x1, int y1, int tile) {
    x0 = std::max(x0, 0);
    y0 = std::max(y0, 0);
    x1 = std::min(x1, w);
    y1 = std::min(y1, h);
    for (int y = y0; y < y1; ++y) {
      for (int x = x0; x < x1; ++x) {
        cells[y][x] = tile;
      }
    }
  }
};

/// One side of the main. Depth d runs away from the main, d = 0 next to the
/// barrier; negative d is inside the barrier.
struct Side {
  int edge;  ///< y where depth 0 starts
  int dir;   ///< -1 = above the main, +1 = below
  int depth; ///< full depth available
  void fill(Grid &g, int x0, int d0, int x1, int d1, int tile) const {
    if (dir > 0) {
      g.fill(x0, edge + d0, x1, edge + d1, tile);
    } else {
      g.fill(x0, edge - d1, x1, edge - d0, tile);
    }
  }
};

/// Split [0, width) into panels of panelW with a barrier between; a short
/// remainder joins the last panel.
std::vector<Panel> splitPanels(int width, int panelW, int bw) {
  std::vector<Panel> panels;
  int x = 0;
  while (x < width) {
    int end = x + panelW;
    if (width - (end + bw) < panelW / 2) {
      end = width;
    }
    Panel p;
    p.x0 = x;
    p.x1 = end;
    panels.push_back(p);
    x = end + bw;
  }
  return panels;
}

} // namespace

TileMap CaveMine::generate(int width, int height,
                           const GenerationParams &params) {
  const MineParams &mp = params.mine;
  RNG::RandUniversal rng(params.seed);

  // The main runs the long axis: a tall map is built wide, then transposed.
  const bool tall = height > width;
  const int W = tall ? height : width;
  const int H = tall ? width : height;
  Grid g(W, H);

  const int ew = std::max(kMinEntryWidth, mp.mEntryWidth);
  const int ps = std::max(1, mp.mPillarSize);
  const int pl = std::max(1, mp.mPillarLength);
  const int bw = std::max(1, mp.mBarrierWidth);
  const int pitchX = pl + ew; // main cross-cuts, panel cross-cut rows
  const int pitchY = ps + ew; // main entry rows, panel entry columns

  const int hallSize = std::max(kMinHall, mp.mHallSize);

  // Clamp to fit: the main is tall enough for a hall, then has fewer entries
  // (not below that) until each side holds a panel.
  auto mainHeight = [&](int m) { return m * ew + (m - 1) * ps; };
  auto sideDepth = [&](int m) { return (H - mainHeight(m) - 2 * bw) / 2; };
  int minEntries = 1;
  while (mainHeight(minEntries) < kMinHall) {
    ++minEntries;
  }
  int entries = std::max(minEntries, mp.mMainEntries);
  while (entries > minEntries &&
         sideDepth(entries) < kMinPanelRows * pitchX) {
    --entries;
  }
  const int mainH = std::min(mainHeight(entries), H);
  const int mainTop = (H - mainH) / 2;

  // Main: entry rows the full length, cross-cuts every pitchX.
  for (int e = 0; e < entries; ++e) {
    const int y = mainTop + e * pitchY;
    g.fill(0, y, W, y + ew, FLOOR);
  }
  const int phase = rng.getInt(0, pitchX - 1);
  for (int x = phase - pitchX; x < W; x += pitchX) {
    g.fill(x, mainTop, x + ew, mainTop + mainH, FLOOR);
  }
  // Main halls: pillars cleared over hallSize, the full main height.
  const int hallSpacing = std::max(hallSize + 1, mp.mMainHallSpacing);
  for (int x = rng.getInt(0, hallSpacing - 1) - hallSpacing; x < W;
       x += hallSpacing) {
    g.fill(x, mainTop, x + hallSize, mainTop + mainH, FLOOR);
  }

  // Panel type shares; scaled down if they add up to more than 1.
  float robbed = std::max(0.0f, mp.mRobbedShare);
  float longwall = std::max(0.0f, mp.mLongwallShare);
  float unmined = std::max(0.0f, mp.mUnminedShare);
  const float shareSum = robbed + longwall + unmined;
  if (shareSum > 1.0f) {
    robbed /= shareSum;
    longwall /= shareSum;
    unmined /= shareSum;
  }
  const float seamMin = std::clamp(mp.mSeamDepthMin, 0.0f, 1.0f);
  const int panelMin = std::max(pitchY + ew, kMinHall);
  const int panelW =
      std::clamp(mp.mPanelWidth, panelMin, std::max(panelMin, W / 2));

  const Side sides[2] = {
      {mainTop - bw, -1, mainTop - bw},
      {mainTop + mainH + bw, +1, H - (mainTop + mainH + bw)}};

  for (const Side &side : sides) {
    const int fullRows = side.depth / pitchX;
    if (fullRows < kMinPanelRows) {
      continue;
    }
    std::vector<Panel> panels = splitPanels(W, panelW, bw);

    // Layout and type of each panel.
    bool anyMined = false;
    for (Panel &p : panels) {
      const int span = p.x1 - p.x0;
      p.cols = (span - ew) / pitchY + 1;
      p.extent = (p.cols - 1) * pitchY + ew;
      p.colStart = p.x0 + (span - p.extent) / 2;
      const double keep = seamMin + (1.0 - seamMin) * rng.getFloat();
      const int rows = static_cast<int>(side.depth * keep) / pitchX;
      p.rows = std::clamp(rows, kMinPanelRows, fullRows);
      const double roll = rng.getFloat();
      p.type = roll < robbed                        ? PanelType::ROBBED
               : roll < robbed + longwall           ? PanelType::LONGWALL
               : roll < robbed + longwall + unmined ? PanelType::UNMINED
                                                    : PanelType::PILLAR;
      anyMined = anyMined || p.type != PanelType::UNMINED;
    }
    if (!anyMined) {
      panels.front().type = PanelType::PILLAR;
    }

    // Carve each panel, then its openings into the main.
    for (const Panel &p : panels) {
      if (p.type == PanelType::UNMINED) {
        continue;
      }
      const int depth = p.rows * pitchX;
      if (p.type == PanelType::LONGWALL) {
        side.fill(g, p.colStart, 0, p.colStart + p.extent, depth, FLOOR);
      } else {
        for (int j = 0; j < p.cols; ++j) {
          const int cx = p.colStart + j * pitchY;
          side.fill(g, cx, 0, cx + ew, depth, FLOOR);
        }
        for (int k = 0; k < p.rows; ++k) {
          const int d = k * pitchX + pl;
          side.fill(g, p.colStart, d, p.colStart + p.extent, d + ew, FLOOR);
        }
      }
      if (p.type == PanelType::ROBBED) {
        for (int k = 0; k < p.rows; ++k) {
          for (int j = 0; j + 1 < p.cols; ++j) {
            if (rng.getFloat() >= mp.mRobChance) {
              continue;
            }
            const int x0 = p.colStart + j * pitchY + ew;
            const int d0 = k * pitchX;
            side.fill(g, x0, d0, x0 + ps, d0 + pl, FLOOR);
            if (rng.getFloat() < mp.mRubbleChance && ps - 2 >= kMinStubSide &&
                pl - 2 >= kMinStubSide) {
              side.fill(g, x0 + 1, d0 + 1, x0 + ps - 1, d0 + pl - 1, WALL);
            }
          }
        }
      }

      // Openings: distinct entry columns, straight through the barrier.
      std::vector<int> colIdx(static_cast<size_t>(p.cols));
      for (int j = 0; j < p.cols; ++j) {
        colIdx[static_cast<size_t>(j)] = j;
      }
      const int openings = std::min(p.cols, std::max(1, mp.mBarrierOpenings));
      for (int o = 0; o < openings; ++o) {
        const int pick = rng.getInt(o, p.cols - 1);
        std::swap(colIdx[static_cast<size_t>(o)],
                  colIdx[static_cast<size_t>(pick)]);
        const int cx = p.colStart + colIdx[static_cast<size_t>(o)] * pitchY;
        side.fill(g, cx, -bw, cx + kGateWidth, 0, FLOOR);
      }

      // Panel hall at the mouth, on the first opening, so the panel seeds
      // its own room. A LONGWALL panel is a hall already.
      if (p.type != PanelType::LONGWALL) {
        const int cx = p.colStart + colIdx[0] * pitchY + kGateWidth / 2;
        const int hallW = std::min(hallSize, p.x1 - p.x0);
        const int hx = std::clamp(cx - hallW / 2, p.x0, p.x1 - hallW);
        side.fill(g, hx, 0, hx + hallW, std::min(hallSize, depth), FLOOR);
      }
    }

    // Side openings between mined neighbours, on a shared cross-cut row.
    for (size_t i = 0; i + 1 < panels.size(); ++i) {
      const Panel &a = panels[i];
      const Panel &b = panels[i + 1];
      if (a.type == PanelType::UNMINED || b.type == PanelType::UNMINED) {
        continue;
      }
      if (rng.getFloat() >= mp.mSideOpenChance) {
        continue;
      }
      const int k = rng.getInt(0, std::min(a.rows, b.rows) - 1);
      const int d = k * pitchX + pl;
      side.fill(g, a.colStart + a.extent, d, b.colStart, d + kGateWidth, FLOOR);
    }
  }

  if (!tall) {
    return std::move(g.cells);
  }
  TileMap out(height, std::vector<int>(width, WALL));
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      out[y][x] = g.cells[x][y];
    }
  }
  return out;
}

} // namespace Cave
