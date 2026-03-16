#ifndef GENERATION_PARAMS_H
#define GENERATION_PARAMS_H

#include <vector>

namespace Cave {

enum class CaveType { CELLULAR, VORONOI, HEIGHTMAP, DLA, BSP_TECTONIC, EMPTY };

struct GenerationStep {
  int b3_min, b3_max;
  int b5_min, b5_max;
  int s3_min, s3_max;
  int s5_min, s5_max;
  int reps;
};

struct CellularParams {
  int mOctaves = 8;
  bool mPerlin = false;
  float mWallChance = 0;
  float mFreq = 1;
  float mAmp = 1;
  std::vector<GenerationStep> mGenerations;
};

struct VoronoiParams {
  float mNoiseScale = 0.02f;
  float mWarpStrength = 30.0f;
  float mVoronoiWeight = 0.35f;
  float mThreshold = 0.48f;
  bool mInvertVoronoi = false;
};

/**
 * @struct HeightmapParams
 * @brief Parameters for Fractal Heightmap cave generation.
 * @details SPECIFICATION: Generates an organic cave layout using 3D Simplex
 * noise based on a "water level" cut-off where below is floor and above is
 * wall.
 */
struct HeightmapParams {
  int mOctaves = 6;
  float mFreq = 1.0f;
  float mWaterLevelMin = -0.1f;
  float mWaterLevelMax = 0.1f;
};

/**
 * @struct DLAParams
 * @brief Parameters for Diffusion-Limited Aggregation (DLA) cave generation.
 * @details SPECIFICATION: Configures the particle simulation count and
 * number/size of carved open areas in the resulting structure. mParticleRadius
 * controls the carve radius per crystallized particle (0 = single cell).
 */
struct DLAParams {
  int mParticleCount = 3000;
  int mParticleRadius = 0;
  int mOpenAreaCount = 5;
  int mOpenAreaRadius = 3;
};

/**
 * @struct TunnelParams
 * @brief Parameters controlling how rooms are connected by tunnels.
 *
 * @details
 * Tunnels shorter than mMinLengthForOrganic use a straight Bresenham line
 * (1-tile wide). Longer tunnels use the organic path algorithm which walks
 * step-by-step along the straight route, deviating the centreline and varying
 * the carved circle radius at each step.
 *
 * WIGGLE — lateral deviation of the tunnel centreline:
 *   mWiggleAmplitude  Max lateral shift in tiles at the tunnel midpoint.
 *                     Offset = noise(x*freq, y*freq+seed) * amplitude * sin(t*PI)
 *                     0   = dead-straight centreline
 *                     0.5-1 = gentle, barely noticeable curve
 *                     3+  = erratic / wanders far from the straight path
 *   mWiggleFrequency  Spatial frequency of the oscillation along the path.
 *                     Higher = more rapid direction changes within one tunnel.
 *
 * PULSE — carved circle radius at each step:
 *   Base radius is always 1.0 tile. mWidthPulseAmplitude adds to it:
 *     radius = 1.0 + |noise| * mWidthPulseAmplitude
 *   mWidthPulseAmplitude  Max extra radius in tiles (0 = uniform 1-tile circles).
 *                         0.1  = circles up to 1.1 radius = ~2.2 tile diameter
 *                         1.2  = circles up to 2.2 radius = ~4.4 tile diameter
 *   mWidthPulseFrequency  How fast the radius oscillates along the path length.
 *
 * EFFECTIVE CARVED WIDTH (worst case at tunnel midpoint):
 *   max_width = 2 * (1.0 + mWidthPulseAmplitude + mWiggleAmplitude) tiles
 *   e.g. amplitude=0.8, pulse=0.1  →  max_width ≈  3.8 tiles  (thin, organic)
 *        amplitude=3.0, pulse=1.2  →  max_width ≈ 10.4 tiles  (destroys large maps)
 *
 * EXTRA CONNECTIONS — non-MST loop-closing tunnels:
 *   After the MST connects all rooms, each room may draw extra tunnels to its
 *   nearest non-MST neighbours (sorted by distance, gated by chance).
 *   mExtraConnections       Max extra tunnels per room (0 = MST only).
 *   mExtraConnectionChance  0..1 per-room probability of attempting extras.
 *   mExtraConnectionMaxDist Euclidean tile distance cap for candidates.
 *                           Only rooms within this distance are considered.
 *                           0 = no distance limit (may draw cross-map tunnels).
 */
struct TunnelParams {
  int mMinLengthForOrganic = 5;
  float mWiggleAmplitude = 3.0f;
  float mWiggleFrequency = 0.2f;
  float mWidthPulseAmplitude = 1.2f;
  float mWidthPulseFrequency = 0.3f;
  int mExtraConnections = 0;
  float mExtraConnectionChance = 1.0f;
  int mExtraConnectionMaxDist = 20; // 0 = unlimited
};

/**
 * @struct BspTectonicParams
 * @brief Parameters for BSP Tectonic (Geode style) cave generation.
 * @details SPECIFICATION: Configures the recursive BSP splitting and the
 * internal Cellular Automata parameters used for the individual rooms.
 */
struct BspTectonicParams {
  int mBspDepth = 4;
  int mMinRoomSize = 16;
  int mRoomPadding = 2;
  CellularParams mCaParams;
};

struct GenerationParams {
  CaveType mCaveType = CaveType::CELLULAR;
  int seed = 0;

  CellularParams cellular;
  VoronoiParams voronoi;
  HeightmapParams heightmap;
  DLAParams dla;
  TunnelParams tunnel;
  BspTectonicParams bspTectonic;
};

} // namespace Cave

#endif
