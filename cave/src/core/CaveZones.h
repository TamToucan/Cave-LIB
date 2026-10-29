#ifndef CAVE_ZONES_H
#define CAVE_ZONES_H

#include "GenerationParams.h"
#include "TileTypes.h"

namespace Cave {

/**
 * @class CaveZones
 * @brief Mixed-zones generator: a layout of slots, each run by another
 * backend.
 * @details SPECIFICATION: specs/features/cave_gen_zones.md. The layout
 * (GAUNTLET, LAIR, HARD_CENTRE, QUAD) splits the map into slots, falling back
 * to a simpler layout when a slot would be under kMinSlot. Each slot runs the
 * backend picked from the slot type list with a seed offset per slot; wall
 * borders separate slots and straight joins cross each border. joinRooms in
 * Cave repairs any piece left apart. Never runs ZONES, DLA or EMPTY in a
 * slot.
 */
class CaveZones {
public:
  /// Layout values for ZonesParams::mLayout.
  enum Layout { GAUNTLET = 0, LAIR = 1, HARD_CENTRE = 2, QUAD = 3 };

  /// Generator types a slot may run, in slot-type knob order.
  static constexpr CaveType kSlotTypes[] = {
      CaveType::CELLULAR,     CaveType::VORONOI, CaveType::HEIGHTMAP,
      CaveType::BSP_TECTONIC, CaveType::MINE,    CaveType::CITY,
      CaveType::VEINS};
  /// Number of entries in kSlotTypes.
  static constexpr int kSlotTypeCount =
      static_cast<int>(sizeof(kSlotTypes) / sizeof(kSlotTypes[0]));

  /**
   * @brief Generate a zones TileMap of size width x height (WALL / FLOOR).
   * @param params Reads ZonesParams (params.zones), params.seed, and each slot
   * generator's own params block.
   */
  static TileMap generate(int width, int height,
                          const GenerationParams &params);
};

} // namespace Cave

#endif
