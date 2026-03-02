#ifndef GENERATION_PARAMS_H
#define GENERATION_PARAMS_H

#include <vector>

namespace Cave {

enum class CaveType { CELLULAR, VORONOI, HEIGHTMAP, DLA, BSP_TECTONIC };

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
 * number/size of carved open areas in the resulting structure.
 */
struct DLAParams {
  int mParticleCount = 3000;
  int mOpenAreaCount = 5;
  int mOpenAreaRadius = 3;
};

struct TunnelParams {
  int mMinLengthForOrganic =
      5; // If Manhattan thickness > this, use organic math
  float mWiggleAmplitude = 3.0f; // Float used for smooth Perlin multiplication
                                 // before rounding to int
  float mWiggleFrequency = 0.2f; // How fast the tunnel sways laterally
  float mWidthPulseAmplitude =
      1.2f; // Max random thickness added to base tunnel
  float mWidthPulseFrequency = 0.3f; // How fast the thickness pulses
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
