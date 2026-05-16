#ifndef CAVE_SMOOTHER_H
#define CAVE_SMOOTHER_H

/**
 * @file CaveSmoother.h
 * @brief Post-processing pass that rounds corners and removes diagonal gaps.
 * @details Operates in-place on a TileMap of WALL/FLOOR cells. Run after the
 * generator backend has finished and rooms have been joined. CaveInfo flags
 * control which sub-stages execute (edges, corners, points).
 */

#include <cstddef>
#include <vector>

#include "CaveInfo.h"
#include "TileTypes.h"

namespace Cave {

/// Pattern-match descriptor consumed by smoothTheGrid (defined in .cpp).
struct UpdateInfo;

/**
 * @class CaveSmoother
 * @brief In-place smoothing of a TileMap: corner rounding, gap removal.
 */
class CaveSmoother {
  /// Convert diagonal wall pairs into solid corners so paths stay walkable.
  void removeDiagonalGaps();
  /// Replace straight wall edges with sloped tiles where applicable.
  void smoothEdges(std::vector<std::vector<bool>> &smoothedGrid);
  /// Round outward/inward corners by inserting CORNR_* and DEND_* tiles.
  void smoothCorners(std::vector<std::vector<bool>> &smoothedGrid);
  /// Collapse single isolated wall/floor points that break smoothing patterns.
  void smoothPoints();
  /**
   * @brief Generic pattern-rewriting pass over the grid.
   * @param updateInfos Array of UpdateInfo describing match/replace rules.
   * @param updateInGrid If true, rewrites flow back into inGrid for chaining.
   * @return True if any cell changed (caller may iterate to fixed point).
   */
  template <size_t SZ>
  bool smoothTheGrid(UpdateInfo (&updateInfos)[SZ],
                     std::vector<std::vector<int>> &inGrid,
                     std::vector<std::vector<bool>> &smoothedGrid,
                     bool updateInGrid = false);

public:
  CaveSmoother(TileMap &tm, const CaveInfo &i);
  ~CaveSmoother();

  /// Run all enabled smoothing stages in order on the bound TileMap.
  void smooth();

private:
  TileMap &mTileMap;
  const CaveInfo &mInfo;
};

} // namespace Cave

#endif
