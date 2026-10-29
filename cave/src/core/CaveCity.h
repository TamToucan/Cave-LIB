#ifndef CAVE_CITY_H
#define CAVE_CITY_H

#include "GenerationParams.h"
#include "TileTypes.h"

namespace Cave {

/**
 * @class CaveCity
 * @brief Buried-city generator (box version of DCSS layout_chaotic_city).
 * @details SPECIFICATION: specs/features/cave_gen_city.md. Wall split lines
 * cut the map into a grid of wards joined by 2-wide gates. Each ward holds a
 * plaza hall and solid blocks, each with a street ring and grown to close
 * the gaps; some blocks are hollow buildings with doors. Connected by construction: no block touches
 * another block's ring or the ward edge. The plaza and the 2-wide gates make
 * each ward a separate room for the game's room finder. Integer geometry;
 * one RNG stream in a fixed order.
 */
class CaveCity {
public:
  /**
   * @brief Generate a city TileMap of size width x height (WALL / FLOOR).
   * @param params Reads CityParams (params.city) and params.seed.
   */
  static TileMap generate(int width, int height,
                          const GenerationParams &params);
};

} // namespace Cave

#endif
