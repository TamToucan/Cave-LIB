#include <iostream>
#include <vector>

#include "Cave.h"
#include "CaveInfo.h"
#include "GenerationParams.h"
#include "TileTypes.h"

int main() {
  Cave::CaveInfo info;
  Cave::GenerationParams params;

  params.seed = 123;
  params.cellular.mOctaves = 8;
  params.cellular.mPerlin = false;
  params.cellular.mWallChance = 0.60;
  params.cellular.mFreq = 16.7;
  params.mCaveType = Cave::CaveType::CELLULAR;

  Cave::GenerationStep step;
  step.b3_min = 2; // need 2 neighbours
  step.b3_max = 5;
  step.b5_min = 2;
  step.b5_max = 5;
  step.s3_min = 4;
  step.s3_max = 8; // to survive, need 4 friends
  step.s5_min = 4;
  step.s5_max = 8;
  step.reps = 3;

#if 0
  // Generation parameters
  params.seed = 424242;
  params.mOctaves = 1;
  params.mPerlin = false;
  params.mWallChance = 0.65;
  params.mFreq = 13.7;

  Cave::GenerationStep step;
  step.b3_min = 3;
  step.b3_max = 4;
  step.b5_min = 12;
  step.b5_max = 16;
  step.s3_min = 2;
  step.s3_max = 5;
  step.s5_min = 10;
  step.s5_max = 14;
  step.reps = 2;
#endif

  params.cellular.mGenerations.push_back(step);

  info.mCaveWidth = 64;
  info.mCaveHeight = 32;
  info.mBorderWidth = 1;
  info.mBorderHeight = 1;
  info.mCellWidth = 8;  // NOT actually needed, GDCave thing
  info.mCellHeight = 8; // NOT actually needed, GDCave thing
  info.mSmoothing = false;

  Cave::Cave cave(info, params);
  // Generate the cave
  Cave::TileMap tileMap = cave.generate();

  // Print the tile map to the console
  for (int y = 0; y < tileMap.size(); ++y) {
    for (int x = 0; x < tileMap[0].size(); ++x) {
      std::cout << ((tileMap[y][x] == Cave::FLOOR) ? ' ' : '#');
    }
    std::cout << std::endl;
  }

  return 0;
}
