#ifndef CAVE_BSP_TECTONIC_H
#define CAVE_BSP_TECTONIC_H

#include "GenerationParams.h"
#include "TileTypes.h"

namespace Cave {

/**
 * @class CaveBspTectonic
 * @brief BSP Tectonic (Geode) Cave Generator
 * @details SPECIFICATION: Generates an organic cave layout composed of
 * isolated rooms using a BSP tree. Each BSP leaf is hollowed out randomly
 * using a localized Cellular Automata step. Connecting narrow passages are
 * handled organically by Cave's graph join logic later.
 */
class CaveBspTectonic {
public:
  /**
   * @brief Generate a BSP Tectonic TileMap of size width x height.
   * @param params Pulls BspTectonicParams for split depth, room size and the
   * embedded CellularParams used to hollow each leaf.
   */
  static TileMap generate(int width, int height,
                          const GenerationParams &params);
};

} // namespace Cave

#endif
