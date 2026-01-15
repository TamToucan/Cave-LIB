#ifndef CAVE_SMOOTHER_H
#define CAVE_SMOOTHER_H

#include "CaveInfo.h"
#include "TileTypes.h"

namespace Cave {

class CaveSmoother {
  void smoothEdges(std::vector<std::vector<int>>& smoothedGrid);
  void smoothCorners(std::vector<std::vector<int>>& smoothedGrid);

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
