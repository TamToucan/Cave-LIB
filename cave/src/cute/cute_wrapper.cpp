#include "cute.h"
#include "Cave.h"
#include "CaveInfo.h"
#include "GenerationParams.h"
#include <iostream>

#ifdef _WIN32
  #define CAVE_CUTE_EXPORT __declspec(dllexport)
#else
  #define CAVE_CUTE_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {
    CAVE_CUTE_EXPORT void cute_wrapper_test() {
        Cave::CaveInfo info;
        info.mCaveWidth = 100;
        info.mCaveHeight = 100;

        Cave::GenerationParams params;
        params.seed = 12345;
        params.mWallChance = 0.45f;

        Cave::Cave cave(info, params);
        auto map = cave.generate();

        std::cout << "Cave generated with size: " << map.size() << std::endl;
    }
}
