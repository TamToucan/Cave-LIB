#include "CaveVeins.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <vector>

#include "RandUniversal.h"

namespace Cave {

namespace {

constexpr int kMaxGrowSteps = 2000; ///< VEIN-02
constexpr int kRootTries = 8;       ///< roots: best of this many cells
constexpr float kZeroSum = 1e-3f;   ///< pull sum treated as zero below this
constexpr int kLoopPathFactor = 4;  ///< loop partner tree path >= this * loopDist
constexpr int kMinWidth = 2;        ///< VEIN-04 (GC-05)
constexpr float kHashCell = 4.0f;   ///< node hash cell side (VEIN-03)

struct Vec {
  float x;
  float y;
};

struct Node {
  Vec p;
  int parent; ///< -1 = root
  int root;   ///< index of its root node
  float depth; ///< tree path length from the root
};

/// Uniform grid of node indices, cell side = cell. Cells keep insert order.
struct Hash {
  float cell;
  int cw;
  int ch;
  std::vector<std::vector<int>> cells;
  Hash(int w, int h, float c)
      : cell(c), cw(static_cast<int>(w / c) + 1), ch(static_cast<int>(h / c) + 1),
        cells(static_cast<size_t>(cw * ch)) {}
  int cx(float x) const { return std::clamp(static_cast<int>(x / cell), 0, cw - 1); }
  int cy(float y) const { return std::clamp(static_cast<int>(y / cell), 0, ch - 1); }
  void add(int i, Vec p) { cells[static_cast<size_t>(cy(p.y) * cw + cx(p.x))].push_back(i); }
  /// Nearest node within reach of p (ties: lower index); -1 if none.
  /// Searches rings of cells outward and stops once every cell left is
  /// farther than the best found, so the answer does not depend on the cell.
  template <typename Ok>
  int nearest(const std::vector<Node> &nodes, Vec p, float reach, Ok ok) const {
    const int span = static_cast<int>(reach / cell) + 1;
    const int x0 = cx(p.x), y0 = cy(p.y);
    const float r2 = reach * reach;
    int best = -1;
    float bestD = 0.0f;
    for (int ring = 0; ring <= span; ++ring) {
      // Cells in this ring are at least (ring - 1) * cell from p.
      const float near = (ring - 1) * cell;
      if (best >= 0 && ring > 0 && bestD < near * near) {
        break;
      }
      for (int y = y0 - ring; y <= y0 + ring; ++y) {
        if (y < 0 || y >= ch) {
          continue;
        }
        const bool edgeRow = y == y0 - ring || y == y0 + ring;
        for (int x = x0 - ring; x <= x0 + ring; x += edgeRow ? 1 : 2 * ring) {
          if (x < 0 || x >= cw) {
            continue;
          }
          visit(nodes, p, r2, ok, x, y, best, bestD);
          if (ring == 0) {
            break;
          }
        }
      }
    }
    return best;
  }

private:
  template <typename Ok>
  void visit(const std::vector<Node> &nodes, Vec p, float r2, Ok ok, int x,
             int y, int &best, float &bestD) const {
    for (int i : cells[static_cast<size_t>(y * cw + x)]) {
      const float dx = nodes[static_cast<size_t>(i)].p.x - p.x;
      const float dy = nodes[static_cast<size_t>(i)].p.y - p.y;
      const float d = dx * dx + dy * dy;
      if (d > r2 || !ok(i)) {
        continue;
      }
      if (best < 0 || d < bestD || (d == bestD && i < best)) {
        best = i;
        bestD = d;
      }
    }
  }
};

float dist2(Vec a, Vec b) {
  const float dx = a.x - b.x, dy = a.y - b.y;
  return dx * dx + dy * dy;
}

/// Floor every cell whose centre is within r of segment a-b.
void capsule(TileMap &m, Vec a, Vec b, float r) {
  const int h = static_cast<int>(m.size()), w = static_cast<int>(m[0].size());
  const int x0 = std::max(0, static_cast<int>(std::min(a.x, b.x) - r) - 1);
  const int x1 = std::min(w - 1, static_cast<int>(std::max(a.x, b.x) + r) + 1);
  const int y0 = std::max(0, static_cast<int>(std::min(a.y, b.y) - r) - 1);
  const int y1 = std::min(h - 1, static_cast<int>(std::max(a.y, b.y) + r) + 1);
  const float abx = b.x - a.x, aby = b.y - a.y;
  const float len2 = abx * abx + aby * aby;
  const float r2 = r * r;
  for (int y = y0; y <= y1; ++y) {
    for (int x = x0; x <= x1; ++x) {
      const Vec c{x + 0.5f, y + 0.5f};
      float t = 0.0f;
      if (len2 > 0.0f) {
        t = std::clamp(((c.x - a.x) * abx + (c.y - a.y) * aby) / len2, 0.0f, 1.0f);
      }
      if (dist2(c, {a.x + t * abx, a.y + t * aby}) < r2) {
        m[static_cast<size_t>(y)][static_cast<size_t>(x)] = FLOOR;
      }
    }
  }
}

} // namespace

TileMap CaveVeins::generate(int width, int height,
                            const GenerationParams &params) {
  const VeinsParams &vp = params.veins;
  RNG::RandUniversal rng(params.seed);
  TileMap map(static_cast<size_t>(height),
              std::vector<int>(static_cast<size_t>(width), WALL));

  // 1. Attractors on a jittered grid.
  const int sp = std::max(1, vp.mAttractorSpacing);
  std::vector<Vec> attr;
  for (int gy = 0; gy < height; gy += sp) {
    for (int gx = 0; gx < width; gx += sp) {
      if (rng.getFloat() >= vp.mAttractorKeep) {
        continue;
      }
      const int x = gx + rng.getInt(0, std::min(sp, width - gx) - 1);
      const int y = gy + rng.getInt(0, std::min(sp, height - gy) - 1);
      attr.push_back({x + 0.5f, y + 0.5f});
    }
  }
  std::vector<char> live(attr.size(), 1);

  // 2. Roots, spread out: best of kRootTries cells by distance to the rest.
  const float kill = std::max(0.5f, vp.mKillDist);
  const float reach = std::max(kill + 0.5f, vp.mAttractDist);
  const float seg = std::max(0.5f, vp.mSegmentLen);
  std::vector<Node> nodes;
  Hash hash(width, height, std::max(kHashCell, kill));
  const int roots = std::max(1, vp.mRootCount);
  for (int r = 0; r < roots; ++r) {
    Vec best{0.0f, 0.0f};
    float bestD = -1.0f;
    const int tries = r == 0 ? 1 : kRootTries;
    for (int t = 0; t < tries; ++t) {
      const Vec c{rng.getInt(0, width - 1) + 0.5f, rng.getInt(0, height - 1) + 0.5f};
      float d = 0.0f;
      for (int k = 0; k < r; ++k) {
        const float dk = dist2(c, nodes[static_cast<size_t>(k)].p);
        d = k == 0 ? dk : std::min(d, dk);
      }
      if (d > bestD) {
        best = c;
        bestD = d;
      }
    }
    nodes.push_back({best, -1, r, 0.0f});
    hash.add(r, best);
  }

  // 3. Grow.
  const size_t nodeCap = static_cast<size_t>(width) * height / 2;
  std::unordered_set<long long> used; // quantised child positions
  const auto key = [width](Vec p) {
    return static_cast<long long>(p.y * 2.0f) * (2LL * width + 2) +
           static_cast<long long>(p.x * 2.0f);
  };
  for (const Node &n : nodes) {
    used.insert(key(n.p));
  }
  const auto any = [](int) { return true; };
  for (int step = 0; step < kMaxGrowSteps; ++step) {
    std::vector<Vec> sum(nodes.size(), Vec{0.0f, 0.0f});
    std::vector<int> first(nodes.size(), -1);
    bool anyLive = false;
    for (size_t a = 0; a < attr.size(); ++a) {
      if (!live[a]) {
        continue;
      }
      anyLive = true;
      const int n = hash.nearest(nodes, attr[a], reach, any);
      if (n < 0) {
        continue;
      }
      const Vec d{attr[a].x - nodes[static_cast<size_t>(n)].p.x,
                  attr[a].y - nodes[static_cast<size_t>(n)].p.y};
      const float len = std::sqrt(d.x * d.x + d.y * d.y);
      if (len <= 0.0f) {
        continue;
      }
      if (first[static_cast<size_t>(n)] < 0) {
        first[static_cast<size_t>(n)] = static_cast<int>(a);
      }
      sum[static_cast<size_t>(n)].x += d.x / len;
      sum[static_cast<size_t>(n)].y += d.y / len;
    }
    if (!anyLive) {
      break;
    }
    const size_t before = nodes.size();
    for (size_t i = 0; i < before && nodes.size() < nodeCap; ++i) {
      if (first[i] < 0) {
        continue;
      }
      Vec d = sum[i];
      float len = std::sqrt(d.x * d.x + d.y * d.y);
      if (len < kZeroSum) {
        const Vec f = attr[static_cast<size_t>(first[i])];
        d = {f.x - nodes[i].p.x, f.y - nodes[i].p.y};
        len = std::sqrt(d.x * d.x + d.y * d.y);
      }
      const Vec c{std::clamp(nodes[i].p.x + seg * d.x / len, 0.5f, width - 0.5f),
                  std::clamp(nodes[i].p.y + seg * d.y / len, 0.5f, height - 0.5f)};
      if (!used.insert(key(c)).second) {
        continue;
      }
      const int idx = static_cast<int>(nodes.size());
      nodes.push_back({c, static_cast<int>(i), nodes[i].root, nodes[i].depth + seg});
      hash.add(idx, c);
    }
    // Kill attractors eaten by any node.
    for (size_t a = 0; a < attr.size(); ++a) {
      if (live[a] && hash.nearest(nodes, attr[a], kill, any) >= 0) {
        live[a] = 0;
      }
    }
    if (nodes.size() == before) {
      break;
    }
  }

  // 4. Width: tips below each node (children have larger indices).
  std::vector<int> tips(nodes.size(), 0);
  for (size_t i = nodes.size(); i-- > 0;) {
    if (tips[i] == 0) {
      tips[i] = 1;
    }
    if (nodes[i].parent >= 0) {
      tips[static_cast<size_t>(nodes[i].parent)] += tips[i];
    }
  }
  int lmax = 1;
  for (int r = 0; r < roots; ++r) {
    lmax = std::max(lmax, tips[static_cast<size_t>(r)]);
  }
  const int minW = std::max(kMinWidth, std::min(vp.mMinWidth, vp.mMaxWidth));
  const int maxW = std::max(minW, vp.mMaxWidth);
  const auto radius = [&](size_t i) {
    return minW / 2.0f +
           (maxW - minW) / 2.0f *
               std::sqrt(static_cast<float>(tips[i]) / static_cast<float>(lmax));
  };

  // 5. Carve.
  std::vector<char> hasChild(nodes.size(), 0);
  for (size_t i = 0; i < nodes.size(); ++i) {
    if (nodes[i].parent < 0) {
      continue;
    }
    hasChild[static_cast<size_t>(nodes[i].parent)] = 1;
    capsule(map, nodes[static_cast<size_t>(nodes[i].parent)].p, nodes[i].p,
            radius(i));
  }

  // 6. Chambers: some tips, every root.
  for (size_t i = 0; i < nodes.size(); ++i) {
    if (nodes[i].parent < 0) {
      capsule(map, nodes[i].p, nodes[i].p, static_cast<float>(maxW));
    } else if (!hasChild[i] && rng.getFloat() < vp.mTipRoomChance) {
      capsule(map, nodes[i].p, nodes[i].p, static_cast<float>(vp.mTipRoomRadius));
    }
  }

  // 7. Loops: link a node to the nearest node far away along the tree.
  const long loops = std::lround(vp.mLoopRate * width * height / 10000.0);
  const float loopDist = std::max(1.0f, vp.mLoopDist);
  for (long l = 0; l < loops && !nodes.empty(); ++l) {
    const int a = rng.getInt(0, static_cast<int>(nodes.size()) - 1);
    const Node &na = nodes[static_cast<size_t>(a)];
    const auto farAlongTree = [&](int b) {
      const Node &nb = nodes[static_cast<size_t>(b)];
      if (nb.root != na.root) {
        return true;
      }
      // Lowest common ancestor: walk the deeper index up (parents < children).
      int x = a, y = b;
      while (x != y) {
        if (x > y) {
          x = nodes[static_cast<size_t>(x)].parent;
        } else {
          y = nodes[static_cast<size_t>(y)].parent;
        }
      }
      const float path = na.depth + nb.depth - 2.0f * nodes[static_cast<size_t>(x)].depth;
      return path >= kLoopPathFactor * loopDist;
    };
    const int b = hash.nearest(nodes, na.p, loopDist, farAlongTree);
    if (b >= 0) {
      capsule(map, na.p, nodes[static_cast<size_t>(b)].p, minW / 2.0f);
    }
  }
  return map;
}

} // namespace Cave
