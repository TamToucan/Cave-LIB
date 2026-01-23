#ifndef CAVE_SMOOTHER_H
#define CAVE_SMOOTHER_H

#include <cstddef>
#include <vector>

#include "CaveInfo.h"
#include "TileTypes.h"

namespace Cave {

struct UpdateInfo;

class CaveSmoother {
  void removeDiagonalGaps(std::vector<std::vector<bool>>& smoothedGrid);
  void smoothEdges(std::vector<std::vector<bool>>& smoothedGrid);
  void smoothCorners(std::vector<std::vector<bool>>& smoothedGrid);
  void smoothPoints();
  template <size_t SZ>
  void smoothTheGrid(UpdateInfo (&updateInfos)[SZ],
                     std::vector<std::vector<int>>& inGrid,
                     std::vector<std::vector<bool>>& smoothedGrid,
                     bool updateInGrid = false);

 public:
  CaveSmoother(TileMap& tm, const CaveInfo& i);
  ~CaveSmoother();

  void smooth();

 private:
  TileMap& tileMap;
  const CaveInfo& info;
};

}  // namespace Cave

#endif
