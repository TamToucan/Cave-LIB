#ifndef GENERATION_PARAMS_H
#define GENERATION_PARAMS_H

#include <vector>

namespace Cave {

enum class CaveType { CELLULAR, VORONOI };

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

struct GenerationParams {
  CaveType mCaveType = CaveType::CELLULAR;
  int seed = 0;

  CellularParams cellular;
  VoronoiParams voronoi;
  TunnelParams tunnel;
};

} // namespace Cave

#endif
