#ifndef CUTE_CAVE_HPP
#define CUTE_CAVE_HPP

/**
 * @file CuteCave.hpp
 * @brief Cute Framework wrapper around the core Cave generator.
 * @details Exposes a fluent builder (setX(...) returns *this) for configuring
 * CaveInfo + GenerationParams, plus helpers for loading the tile atlas and
 * invoking generation. Use as: CuteCave c; c.setCaveSize(...).setCaveType(...);
 * auto map = c.make_cave(seed);
 */

#include <cute.h>
#include <string>

#include "CaveInfo.h"
#include "GenerationParams.h"
#include "TileTypes.h"

namespace CuteCave {

/**
 * @class CuteCave
 * @brief Fluent-builder wrapper around the core Cave generator.
 */
class CuteCave {
public:
  /**
   * @struct TileAtlas
   * @brief A loaded Cute texture sliced into per-TileName sprites.
   * @details tile_sprites is indexed by Cave::TileName so renderers can look
   * up the sprite for any tile id returned by make_cave().
   */
  struct TileAtlas {
    CF_Texture texture;
    uint64_t base_id;
    int width_in_tiles; // Should be 512 / 64 = 8 for 512x512 (64x64 tiles)
    CF_Sprite tile_sprites[Cave::TileName::TILE_COUNT];
  };

public:
  CuteCave();

  /// Set the cave grid dimensions in tiles.
  CuteCave &setCaveSize(int width, int height);
  /// Set the border thickness in cells (GDCave-only — Cute renderer ignores it).
  CuteCave &setBorderCellSize(int width, int height);
  /// Set per-cell pixel size (GDCave-only — Cute renderer ignores it).
  CuteCave &setCellSize(int width,
                        int height); // NOT actually needed, GDCave thing
  /// Pick which backend Cave::generate() will run.
  CuteCave &setCaveType(Cave::CaveType type);
  /// Set the start-cell hint (currently unused by core generators).
  CuteCave &setStartCell(int x, int y);

  // --- CellularParams setters --------------------------------------------
  /// Cellular: number of noise octaves.
  CuteCave &setOctaves(int octaves);
  /// Cellular: use Perlin (true) or simple random (false) noise seed.
  CuteCave &setPerlin(bool usePerlin);
  /// Cellular: initial wall density [0,1] before CA passes.
  CuteCave &setWallChance(float wallChance);
  /// Cellular: noise frequency multiplier.
  CuteCave &setFreq(float freq);
  /// Cellular: noise amplitude multiplier.
  CuteCave &setAmp(float amp);

  // --- Smoothing toggles -------------------------------------------------
  /// Master switch for the smoothing post-pass (overrides corner/point flags).
  CuteCave &setSmoothing(bool doSmoothing);
  /// Enable corner-rounding sub-stage of the smoother.
  CuteCave &setSmoothCorners(bool doSmoothCorners);
  /// Enable single-point cleanup sub-stage of the smoother.
  CuteCave &setSmoothPoints(bool doSmoothPoints);
  /// Cellular: list of GenerationStep passes to run on the seeded grid.
  CuteCave &setGenerations(std::vector<Cave::GenerationStep> gens);

  /// Configure VoronoiParams. See VoronoiParams in GenerationParams.h.
  CuteCave &setVoronoiParams(float noiseScale, float warpStrength,
                             float voronoiWeight, float threshold,
                             bool invertVoronoi);
  /// Configure HeightmapParams. See HeightmapParams in GenerationParams.h.
  CuteCave &setHeightmapParams(int octaves, float freq, float waterLevelMin,
                               float waterLevelMax);
  /// Configure DLAParams. See DLAParams in GenerationParams.h.
  CuteCave &setDLAParams(int particleCount, int particleRadius,
                         int openAreaCount, int openAreaRadius);
  /// Configure TunnelParams (room-joining tunnels). See TunnelParams in GenerationParams.h.
  CuteCave &setTunnelParams(int minLengthForOrganic, float wiggleAmplitude,
                            float wiggleFrequency, float widthPulseAmplitude,
                            float widthPulseFrequency, int extraConnections = 0,
                            float extraConnectionChance = 1.0f,
                            int extraConnectionMaxDist = 20);
  /// Configure BSP Tectonic split depth and room sizing.
  CuteCave &setBspTectonicParams(int depth, int minRoomSize, int roomPadding);
  /// Configure the per-leaf CellularParams used inside BSP Tectonic rooms.
  CuteCave &setBspTectonicCaParams(int octaves, bool usePerlin,
                                   float wallChance, float freq, float amp,
                                   std::vector<Cave::GenerationStep> gens);

  /// Load a CF tile-atlas PNG and slice it into per-tile CF_Sprites.
  TileAtlas loadTileAtlas(const char *virtual_path, int tile_size);

  /// Run Cave::generate() with the configured params; returns the TileMap.
  const Cave::TileMap make_cave(int seed);

  /// True if the generated TileMap contains at least one FLOOR cell.
  bool hasFloorSpace(const Cave::TileMap &tileMap) const;
  /// Human-readable dump of the current params (for debug overlays).
  std::string getParamsString() const;

private:
  Cave::CaveInfo m_info;
  Cave::GenerationParams m_gen_params;
};

} // namespace CuteCave

#endif // CUTE_CAVE_HPP
