#ifndef GENERATION_PARAMS_H
#define GENERATION_PARAMS_H

/**
 * @file GenerationParams.h
 * @brief Aggregates all tuneable parameters for cave generation backends.
 * @details GenerationParams is the single bag of settings handed to Cave().
 * mCaveType selects which backend runs; per-backend sub-structs
 * (CellularParams, VoronoiParams, HeightmapParams, DLAParams,
 * BspTectonicParams, MineParams, CityParams, VeinsParams, ZonesParams) hold the parameters for that backend. TunnelParams is
 * shared — it controls how rooms detected after the backend runs are
 * connected.
 */

#include <vector>

namespace Cave {

/// Generator backend selector. Picks which algorithm Cave::generate() runs.
/// EMPTY stays last: clients size per-type tables as EMPTY + 1.
enum class CaveType {
  CELLULAR,
  VORONOI,
  HEIGHTMAP,
  DLA,
  BSP_TECTONIC,
  MINE,
  CITY,
  VEINS,
  ZONES,
  EMPTY
};

/**
 * @struct GenerationStep
 * @brief One iteration of cellular-automata rules.
 * @details b3/b5 thresholds control "birth" (wall created), s3/s5 control
 * "survive" (wall persists) using 3x3 and 5x5 neighbour counts respectively.
 * reps is the number of times this step is applied before moving to the next.
 */
struct GenerationStep {
  int b3_min, b3_max;
  int b5_min, b5_max;
  int s3_min, s3_max;
  int s5_min, s5_max;
  int reps;
};

/**
 * @struct CellularParams
 * @brief Parameters for the classic Cellular-Automata cave generator.
 * @details Seeded with Perlin or simple random noise (mPerlin), with wall
 * density mWallChance and noise frequency/amplitude. mGenerations defines the
 * sequence of CA passes applied to the seed.
 */
struct CellularParams {
  int mOctaves = 8;
  bool mPerlin = false;
  float mWallChance = 0;
  float mFreq = 1;
  float mAmp = 1;
  std::vector<GenerationStep> mGenerations;
};

/**
 * @struct VoronoiParams
 * @brief Parameters for the Voronoi-noise cave generator.
 * @details Combines warped Worley/Voronoi distance with a noise threshold to
 * cut chambers and connecting cracks.
 */
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

/**
 * @struct MineParams
 * @brief Parameters for the room-and-pillar mine generator (CaveMine).
 * @details SPECIFICATION: specs/features/cave_gen_mine.md. Lengths in tiles,
 * shares 0..1. A main of parallel entries runs the long axis; walled panels
 * open off both sides, each PILLAR, ROBBED, LONGWALL or UNMINED. Halls and
 * 2-wide openings make each panel and the main a room for the room finder.
 */
struct MineParams {
  int mEntryWidth = 3;          ///< tunnel width, main and panels
  int mPillarSize = 4;          ///< pillar side across the entries
  int mPillarLength = 6;        ///< pillar side along the entries
  int mMainEntries = 3;         ///< parallel entries in the main
  int mPanelWidth = 32;         ///< panel width along the main
  int mBarrierWidth = 3;        ///< solid wall between panels / main
  int mBarrierOpenings = 2;     ///< openings per panel into the main
  float mSideOpenChance = 0.3f; ///< opening between neighbour panels
  float mRobbedShare = 0.3f;
  float mLongwallShare = 0.15f;
  float mUnminedShare = 0.1f;
  float mRobChance = 0.5f;      ///< per pillar, ROBBED panels
  float mRubbleChance = 0.3f;   ///< removed pillar leaves a crushed stub
  float mSeamDepthMin = 0.5f;   ///< least share of full panel depth kept
  int mHallSize = 9;            ///< panel hall side, main hall length (>= 7)
  int mMainHallSpacing = 48;    ///< main hall every this many tiles
};

/**
 * @struct CityParams
 * @brief Parameters for the buried-city generator (CaveCity).
 * @details SPECIFICATION: specs/features/cave_gen_city.md. Lengths in tiles;
 * blockRate = block tries per 1000 ward tiles; shares 0..1. Walled wards
 * joined by 2-wide gates, each with a plaza hall, filled with blocks and
 * hollow buildings.
 */
struct CityParams {
  int mWardSize = 28;         ///< target ward side (with its wall)
  int mWardWall = 2;          ///< wall thickness between wards
  int mWardJitter = 3;        ///< ward split line shift, tiles
  int mPlazaSize = 9;         ///< plaza side, max (min 7)
  int mBlockMin = 4;          ///< block side, min (ordered with mBlockMax)
  int mBlockMax = 12;         ///< block side, max; also the grow cap
  int mStreetWidth = 2;       ///< clear ring round every block
  float mBlockRate = 20.0f;   ///< tries per 1000 ward tiles
  float mHollowShare = 0.2f; ///< blocks (>= 5x5) made hollow
  int mDoorsMax = 2;          ///< doors per hollow building, 1..this
  int mDoorWidth = 2;         ///< 1 = tight chokepoint, max 2
  bool mGridMode = false;     ///< false = chaotic, true = lattice
  int mGridJitter = 1;        ///< lattice slot shift, tiles
};

/**
 * @struct VeinsParams
 * @brief Parameters for the vein-network generator (CaveVeins).
 * @details SPECIFICATION: specs/features/cave_gen_veins.md. Space
 * colonization. Lengths in tiles; loopRate = cross-links per 10000 tiles.
 */
struct VeinsParams {
  int mAttractorSpacing = 6;   ///< jittered-grid square side
  float mAttractorKeep = 0.8f; ///< chance a square gets an attractor
  float mAttractDist = 18.0f;  ///< attractor reach (> killDist)
  float mKillDist = 4.0f;      ///< attractor eaten within this
  float mSegmentLen = 2.0f;    ///< growth step
  int mRootCount = 1;
  int mMinWidth = 2;           ///< tip width (<= maxWidth)
  int mMaxWidth = 6;           ///< trunk width at the largest root
  float mTipRoomChance = 0.15f;
  int mTipRoomRadius = 4;
  float mLoopRate = 2.0f;      ///< cross-links per 10000 tiles; 0 = tree
  float mLoopDist = 10.0f;     ///< cross-link reach
};

/**
 * @struct ZonesParams
 * @brief Parameters for the mixed-zones generator (CaveZones).
 * @details SPECIFICATION: specs/features/cave_gen_zones.md. A layout splits
 * the map into slots; each slot runs another backend; joins cross the wall
 * borders. Slot values come from the other backends' blocks here.
 */
struct ZonesParams {
  int mLayout = 1;                 ///< 0 GAUNTLET, 1 LAIR, 2 HARD_CENTRE, 3 QUAD
  float mSplitJitter = 0.1f;       ///< split shift, share of slot length
  float mCentreSize = 0.4f;        ///< HARD_CENTRE: centre / map side
  int mSlotType[4] = {1, 3, 1, 3}; ///< index into CaveZones' slot type list
  int mBorderWidth = 3;            ///< wall between slots
  int mJoinsPerBorder = 2;         ///< corridors across each border
  int mJoinWidth = 3;              ///< corridor width
};

/**
 * @struct GenerationParams
 * @brief Top-level container of all parameters used by Cave::generate().
 * @details mCaveType picks which backend runs; only the matching sub-struct is
 * read by the backend. tunnel and seed are shared across all backends.
 */
struct GenerationParams {
  CaveType mCaveType = CaveType::CELLULAR;
  int seed = 0;

  CellularParams cellular;
  VoronoiParams voronoi;
  HeightmapParams heightmap;
  DLAParams dla;
  TunnelParams tunnel;
  BspTectonicParams bspTectonic;
  MineParams mine;
  CityParams city;
  VeinsParams veins;
  ZonesParams zones;
};

} // namespace Cave

#endif
