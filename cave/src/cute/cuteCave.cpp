#include <cute.h>
#include "Cave.h"
#include "CaveInfo.h"
#include "TileTypes.h"
#include "GenerationParams.h"

// This class is just a wrapper to prove both libraries can talk to each other
class CuteCave {

public:
    CuteCave(int width, int height) {
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
	    params.mGenerations.push_back(step);
    }
    ~CuteCave() {
    }

    void draw() {
	    Cute::draw_push_color(Cute::color_white());
	    Cute::draw_line(Cute::v2(0,0), Cute::v2(100, 100), 2.0f);


	    // CaveInfo parameters
	    info.mCaveWidth = 32;
	    info.mCaveHeight = 32;
	    info.mBorderWidth = 1;
	    info.mBorderHeight = 1;
	    info.mCellWidth = 8;
	    info.mCellHeight = 8;

	    Cave::Cave cave(info, params);
	    // Generate the cave
	    Cave::TileMap tileMap = cave.generate();


    }

private:
    Cave::CaveInfo info;
    Cave::GenerationParams params;
};
