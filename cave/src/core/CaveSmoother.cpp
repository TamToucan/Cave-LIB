/*
 * CaveSmoother.cpp
 *
 *  Created on: 7 Dec 2024
 *      Author: tam
 */

#include "CaveSmoother.h"

#include <iostream>
#include <vector>

#include "Cave.h"
#include "CaveInfo.h"
#include "Debug.h"
#include "TileTypes.h"


namespace Cave {

//
// There are
// - 4 tiles for the 45 degree slopes (T45_<corner>
// - 4 tiles for the 60 degree 2 vertical tiles (T60_VT_<corner>)
// - 4 tiles for the 60 degree 2 horizontal tiles (T60_HZ_<corner>
//
// Where <corner> is the "solid" corner
// For <corner> number the corners of tile/tile pair clockwise
// from Top Left e.g./ for the 2 tile horizontal pair
//
// 1--|--2
// |  |  |
// 4__|__3
//
// 0000001
// 0001111  => T60_HZ_3 (since 3 is the "solid" corner)
// 1111111
//
namespace {
//
// The TileGrid's are a 4x4 of tiles where
//   X = don't care
//   B = blank
//   S = set
//   N = loc of 1st tile to change
//   M = loc of 2nd tile to change (of loc1 a one tile update)
//   O = loc of 1st tile to change, but the tile is currently blank
//
// YES. 'N' = pos1 and 'M' = pos2. Don't @ me!
//
// The UpdateInfo holds a pointer to the TileGrid and the two tiles to replace
// the 'N' and 'M' with. The updates is then a list of all the updates
// to check for a match with in the order to check them (2 tile updates need
// checked first because the single tile update is more general and will also
// match what should be a 2 tile update)
//
// The createUpdateInfos parses the grid for each update and set the pos1, pos2
// based on NM and the mask/update based on XBS.
// i.e. the pattern is just a friendly way to give a method to populate these
// valuesRoundTileInfo is the machine-friendly format. It has a mask and value
//
// The roundEdges iterates over each edge cell and calc's the value for the
// 4x4 grid to the right and down of it. The list of updatesis then searched
// to find a match and set the tile(s) for each matching update.
//
// Two grids are maintained; inGrid is just a copy of the TileMapLayer so
// we aren' getting the tile atlas all the time to find walls and because
// the actual TileMapLayer is being updates.
// The smoothedGrid is really just a bool of what tiles have been SMOOTHED.
// This stops a tile being updated twice.
//
// We want to check the 4x4 using the top and left border walls and have the
// 4x4 go 'off the side' of right/bottom borders. To do this the two grids
// are larger than the TileMapLayer and the inGrid copy is shifted by 1,1
// e.g. pos 0,0 on the input map is stored at 1,1 in inGrid and we re-adjust
// when setting the tile.
//
const int GRD_W = 4;
const int GRD_H = 4;
const unsigned char X = 'x';
const unsigned char S = 's';
const unsigned char B = 'b';
const unsigned char N = 'n';
const unsigned char M = 'm';
const unsigned char O = 'o';
//
// Two tile updates (30 and 60 slopes)
//
unsigned char TileGrid60b[GRD_H][GRD_W] = {
    {X, S, X, X}, {B, N, S, X}, {B, M, S, X}, {X, B, S, X}};
unsigned char TileGrid60d[GRD_H][GRD_W] = {
    {S, B, X, X}, {S, M, B, X}, {S, N, B, X}, {X, S, X, X}};
unsigned char TileGrid60c[GRD_H][GRD_W] = {
    {X, X, B, S}, {X, B, M, S}, {X, B, N, S}, {X, X, S, X}};
unsigned char TileGrid60a[GRD_H][GRD_W] = {
    {X, S, X, X}, {S, N, B, X}, {S, M, B, X}, {S, B, X, X}};

unsigned char TileGrid30a[GRD_H][GRD_W] = {
    {X, S, S, S}, {S, N, M, B}, {X, B, B, X}, {X, X, X, X}};
unsigned char TileGrid30d[GRD_H][GRD_W] = {
    {X, X, X, X}, {X, B, B, X}, {S, N, M, B}, {X, S, S, S}};
unsigned char TileGrid30c[GRD_H][GRD_W] = {
    {X, X, X, X}, {X, B, B, X}, {B, M, N, S}, {S, S, S, X}};
unsigned char TileGrid30b[GRD_H][GRD_W] = {
    {X, X, X, X}, {S, S, S, X}, {B, M, N, S}, {X, B, B, X}};
//
// Single 45 degree tile updates
//
unsigned char TileGrid45b[GRD_H][GRD_W] = {
    {X, X, S, X}, {X, B, N, S}, {X, X, B, X}, {X, X, X, X}};
unsigned char TileGrid45c[GRD_H][GRD_W] = {
    {X, X, B, X}, {X, B, N, S}, {X, X, S, X}, {X, X, X, X}};
unsigned char TileGrid45d[GRD_H][GRD_W] = {
    {X, B, X, X}, {S, N, B, X}, {X, S, X, X}, {X, X, X, X}};
unsigned char TileGrid45a[GRD_H][GRD_W] = {
    {X, S, X, X}, {S, N, B, X}, {X, B, X, X}, {X, X, X, X}};
//
// End cap tile updates
// - West, North, East, South
//
unsigned char TileGridNDw[GRD_H][GRD_W] = {
    {X, X, B, S}, {X, B, N, S}, {X, X, B, S}, {X, X, X, X}};
unsigned char TileGridNDn[GRD_H][GRD_W] = {
    {X, X, X, X}, {X, B, X, X}, {B, N, B, X}, {S, S, S, X}};
unsigned char TileGridNDe[GRD_H][GRD_W] = {
    {S, B, X, X}, {S, N, B, X}, {S, B, X, X}, {X, X, X, X}};
unsigned char TileGridNDs[GRD_H][GRD_W] = {
    {S, S, S, X}, {B, N, B, X}, {X, B, X, X}, {X, X, X, X}};
//
// Dead-Endtile updates
// - East, South, West, North
//
unsigned char TileGridDEe[GRD_H][GRD_W] = {
    {X, X, S, S}, {X, B, O, S}, {X, X, S, S}, {X, X, X, X}};
unsigned char TileGridDEs[GRD_H][GRD_W] = {
    {X, X, X, X}, {X, B, X, X}, {S, O, S, X}, {S, S, S, X}};
unsigned char TileGridDEw[GRD_H][GRD_W] = {
    {S, S, X, X}, {S, O, B, X}, {S, S, X, X}, {X, X, X, X}};
unsigned char TileGridDEn[GRD_H][GRD_W] = {
    {S, S, S, X}, {S, O, S, X}, {X, B, X, X}, {X, X, X, X}};
//
// Single isolated tile update
//
unsigned char TileGridNGL[GRD_H][GRD_W] = {
    {X, B, X, X}, {B, N, B, X}, {X, B, X, X}, {X, X, X, X}};
//
// A line of 2 walls to put end cap on
//
unsigned char TileGrid2Dn[GRD_H][GRD_W] = {
    {X, B, X, X}, {B, N, B, X}, {X, S, X, X}, {X, X, X, X}};
unsigned char TileGrid2Ds[GRD_H][GRD_W] = {
    {X, X, X, X}, {X, S, X, X}, {B, N, B, X}, {X, B, X, X}};
unsigned char TileGrid2De[GRD_H][GRD_W] = {
    {X, X, B, X}, {X, S, N, B}, {X, X, B, X}, {X, X, X, X}};
unsigned char TileGrid2Dw[GRD_H][GRD_W] = {
    {X, B, X, X}, {B, N, S, X}, {X, B, X, X}, {X, X, X, X}};
//
// Added for bit sticking off end. Not sure why TileRounder doesn't need it
//
unsigned char TileGrid21n[GRD_H][GRD_W] = {
    {X, X, X, X}, {B, B, X, X}, {S, N, B, X}, {S, B, X, X}};
unsigned char TileGrid22n[GRD_H][GRD_W] = {
    {X, X, X, X}, {S, S, B, X}, {B, N, B, X}, {X, B, X, X}};
unsigned char TileGrid23n[GRD_H][GRD_W] = {
    {X, X, X, X}, {X, B, S, X}, {B, N, S, X}, {X, B, B, X}};
unsigned char TileGrid24n[GRD_H][GRD_W] = {
    {X, X, X, X}, {X, B, X, X}, {B, N, B, X}, {B, S, S, X}};

unsigned char TileGrid25n[GRD_H][GRD_W] = {
    {X, X, X, X}, {S, B, X, X}, {S, N, B, X}, {B, B, X, X}};
unsigned char TileGrid26n[GRD_H][GRD_W] = {
    {X, X, X, X}, {B, S, S, X}, {B, N, B, X}, {X, B, X, X}};
unsigned char TileGrid27n[GRD_H][GRD_W] = {
    {X, X, X, X}, {X, B, B, X}, {B, N, S, X}, {X, B, S, X}};
unsigned char TileGrid28n[GRD_H][GRD_W] = {
    {X, X, X, X}, {X, B, X, X}, {B, N, B, X}, {S, S, B, X}};

///////////////////////////////////////////

// a,b,c,d = Corner TL, RT, BR, BL e.g.
// a   b
//  001
//  011   = T45c  ('c' since bottom right set)
//  111
// d   c

// or
//  1000 0000
//  1111 1000   = T30d1 and T30d2
//  1111 1111
// d1   d2
//

////////////////////////////////////////////////////////////////

//
// Pattern is the NMXBS 4x4 grid to us in
// createUpdateInfos to populate the mask/value.offsets
//
// Take the input value of the 4x4 grid for the point being
// checked, apply the mask and compare it to UpdateInfo value.
// If equal then a pattern match has been found. and the
// offset(s) are used on the input put and the tile(s) updated.
//
struct UpdateInfo {
  unsigned char (*pattern)[GRD_W];
  int mask;
  int value;
  // Offsets from the top left corner point being used to
  // create the 4x4 grid of tile1 ad tile2
  int xoff1, yoff1;
  int xoff2, yoff2;
  // The 1 or 2 tiles to update
  TileName t1;
  TileName t2;
};

UpdateInfo updates[] = {
    // Two tiles (30 and 60)
    {TileGrid30a, 0, 0, 0, 0, 0, 0, H30a1, H30a2},
    {TileGrid60b, 0, 0, 0, 0, 0, 0, V60b1, V60b2},
    {TileGrid30c, 0, 0, 0, 0, 0, 0, H30c1, H30c2},
    {TileGrid60d, 0, 0, 0, 0, 0, 0, V60d1, V60d2},

    {TileGrid30b, 0, 0, 0, 0, 0, 0, H30b1, H30b2},
    {TileGrid60c, 0, 0, 0, 0, 0, 0, V60c1, V60c2},
    {TileGrid30d, 0, 0, 0, 0, 0, 0, H30d1, H30d2},
    {TileGrid60a, 0, 0, 0, 0, 0, 0, V60a1, V60a2},
    // single tiles
    {TileGrid45b, 0, 0, 0, 0, 0, 0, T45b, IGNORE},
    {TileGrid45c, 0, 0, 0, 0, 0, 0, T45c, IGNORE},
    {TileGrid45d, 0, 0, 0, 0, 0, 0, T45d, IGNORE},
    {TileGrid45a, 0, 0, 0, 0, 0, 0, T45a, IGNORE},
    // end caps
    {TileGridNDw, 0, 0, 0, 0, 0, 0, FLOOR, IGNORE},
    {TileGridNDe, 0, 0, 0, 0, 0, 0, FLOOR, IGNORE},
    {TileGridNDn, 0, 0, 0, 0, 0, 0, FLOOR, IGNORE},
    {TileGridNDs, 0, 0, 0, 0, 0, 0, FLOOR, IGNORE},
    // The single isolated tile
    {TileGridNGL, 0, 0, 0, 0, 0, 0, SINGLE, IGNORE},
    // 2 vert/horz tiles
    {TileGrid2Dn, 0, 0, 0, 0, 0, 0, END_N, IGNORE},
    {TileGrid2Ds, 0, 0, 0, 0, 0, 0, END_S, IGNORE},
    {TileGrid2De, 0, 0, 0, 0, 0, 0, END_E, IGNORE},
    {TileGrid2Dw, 0, 0, 0, 0, 0, 0, END_W, IGNORE},
    // dead-ends
    {TileGridDEn, 0, 0, 0, 0, 0, 0, DEND_N, IGNORE},
    {TileGridDEs, 0, 0, 0, 0, 0, 0, DEND_S, IGNORE},
    {TileGridDEe, 0, 0, 0, 0, 0, 0, DEND_E, IGNORE},
    {TileGridDEw, 0, 0, 0, 0, 0, 0, DEND_W, IGNORE},
#if 0
    // Bit sticking off end
    { TileGrid21n,  0, 0, 0,0, 0,0, FLOOR, IGNORE},
    { TileGrid22n,  0, 0, 0,0, 0,0, FLOOR, IGNORE},
    { TileGrid23n,  0, 0, 0,0, 0,0, FLOOR, IGNORE},
    { TileGrid24n,  0, 0, 0,0, 0,0, FLOOR, IGNORE},

    { TileGrid25n,  0, 0, 0,0, 0,0, FLOOR, IGNORE},
    { TileGrid26n,  0, 0, 0,0, 0,0, FLOOR, IGNORE},
    { TileGrid27n,  0, 0, 0,0, 0,0, FLOOR, IGNORE},
    { TileGrid28n,  0, 0, 0,0, 0,0, FLOOR, IGNORE}
#endif
};

//
// Use the patterns to calc and modify the updates with mask,value and offsets
//
void createUpdateInfos() {
  LOG_INFO("====================== SMOOTH CREATE UPDATES");
  for (auto& u : updates) {
    const unsigned char (*grid)[GRD_W] = u.pattern;
    int l_mask = 0;
    int l_value = 0;
    int l_xOff1 = -1;
    int l_yOff1 = -1;
    int l_xOff2 = -1;
    int l_yOff2 = -1;
    int s = (GRD_H * GRD_W) - 1;
    for (int r = 0; r < GRD_H; ++r) {
      for (int c = 0; c < GRD_W; ++c) {
        switch (grid[r][c]) {
          case X:
            l_mask |= 0 << s;
            l_value |= 0 << s;
            break;
          case B:
            l_mask |= 1 << s;
            l_value |= 0 << s;
            break;
          case S:
            l_mask |= 1 << s;
            l_value |= 1 << s;
            break;
          case N:
            l_mask |= 1 << s;
            l_value |= 1 << s;
            l_xOff1 = c;
            l_yOff1 = r;
            break;
          case M:
            l_mask |= 1 << s;
            l_value |= 0 << s;
            l_xOff2 = c;
            l_yOff2 = r;
            break;
          default:
            break;
        }
        --s;
      }
    }
    if (l_xOff1 == -1) {
      LOG_ABORT("No tile position. " << u.t1 << "," << u.t2);
    }
    u.mask = l_mask;
    u.value = l_value;
    u.xoff1 = l_xOff1;
    u.yoff1 = l_yOff1;
    // Make P2 = P1 so don't need to check if 1 or 2 tiles being updated
    u.xoff2 = (l_xOff2 == -1) ? l_xOff1 : l_xOff2;
    u.yoff2 = (l_yOff2 == -1) ? l_yOff1 : l_yOff2;
    LOG_DEBUG("UPDATE: msk:" << std::hex << u.mask << " val:" << u.value
                             << std::dec << " of1: " << u.xoff1 << ","
                             << u.yoff1 << " of2: " << u.xoff2 << ","
                             << u.yoff2);
  }
}

}  // namespace

//////////////////////////////////////////////////

CaveSmoother::CaveSmoother(TileMap& tm, const CaveInfo& i)
    : info(i), tileMap(tm) {
  createUpdateInfos();
}

CaveSmoother::~CaveSmoother() {}

//
// Copy the input, create a 4x4 grid value for each pos (bit set = wall)
// and find any matching update(s). For each match set the TileMapLayer
// cell(s) for the 1 or 2 tiles for each update.
//
void CaveSmoother::smoothEdges() {
  //
  // NOTE: So we can do a 4x4 with the top and left edge being the border
  // we shift the maze 0,0 to 1,1. We also make it wider to allow the
  // right and bottom edges to be a border
  //
  LOG_INFO("====================== SMOOTH EDGES");
  std::vector<std::vector<int>> smoothedGrid(
      info.mCaveHeight + GRD_H + 1,
      std::vector<int>(info.mCaveWidth + GRD_W + 1, IGNORE));
  std::vector<std::vector<int>> inGrid(
      info.mCaveHeight + GRD_H + 1,
      std::vector<int>(info.mCaveWidth + GRD_W + 1, SOLID));

  //
  // Copy the current cave
  // NOTE: Translate the cave 0,0 => 1,1 of grids
  //
  for (int y = 0; y < info.mCaveHeight; y++) {
    for (int x = 0; x < info.mCaveWidth; x++) {
      inGrid[y + 1][x + 1] = Cave::isWall(tileMap, x, y) ? SOLID : FLOOR;
    }
  }
  //
  // Smooth the grid
  //
  for (int y = 0; y < info.mCaveHeight - 1; y++) {
    for (int x = 0; x < info.mCaveWidth - 1; x++) {
      // Get the value of the 4x4 grid
      LOG_DEBUG("==MASK value " << x << "," << y);
      int value = 0;
      int shift = (GRD_H * GRD_W) - 1;
      for (int r = 0; r < GRD_H; ++r) {
        for (int c = 0; c < GRD_W; ++c) {
          if (inGrid[y + r][x + c] == SOLID) {
            value |= (1 << shift);
          }
          --shift;
        }
      }
      LOG_DEBUG("==FIND " << x << "," << y << " val:" << std::hex << value
                          << std::dec);

      // Find the matching update(s) for that value
      //
#if 0
            for (const auto& up : updates) {
                if ( (value&up.mask) == up.value) {
					Vector2i pos1{x+ up.xoff1, y+ up.yoff1};
					Vector2i pos2{x+ up.xoff2, y+ up.yoff2};
                	// Ensure not smoothed it already
                	// - can check both pos since p2 == p1 if no 2nd tile
                	if ((smoothedGrid[pos1.y][pos1.x] == IGNORE)
                	 && (smoothedGrid[pos2.y][pos2.x] == IGNORE)) {
                		// Smooth the first (N) tile
                		// - Need to translate the grid pos back to cave pos
						LOG_INFO("SMOOTH " << x << "," << y << " " << up.t1 << "," << up.t2);
						Cave::setCell(tileMap, pos1.x-1,pos1.y-1, up.t1);
                		smoothedGrid[pos1.y][pos1.x] = SMOOTHED;
                		// Check if there is a second (M) tile
                		if (up.t2 != IGNORE) {
                			// Smooth the second (M) tile
                			// - Need to translate the grid pos back to cave pos
							Cave::setCell(tileMap, pos2.x-1,pos2.y-1, up.t2);
                			smoothedGrid[pos2.y][pos2.x] = SMOOTHED;
                		}
                	}
                }
            }
#else
      int idx = 0;
      for (const auto& up : updates) {
        LOG_DEBUG("  NEXT up:" << idx << " msk:" << std::hex << up.mask
                               << " val:" << up.value << " inVal:" << value
                               << " and:" << (value & up.mask) << std::dec);
        if ((value & up.mask) == up.value) {
          Vector2i pos1{x + up.xoff1, y + up.yoff1};
          Vector2i pos2{x + up.xoff2, y + up.yoff2};

          LOG_DEBUG("      FOUND1 up:" << idx << " p1:" << pos1.x << ","
                                       << pos1.y << " p2:" << pos2.x << ","
                                       << pos2.y);
          // Ensure not smoothed it already
          // - can check both pos since p2 == p1 if no 2nd tile
          if ((smoothedGrid[pos1.y][pos1.x] == IGNORE) &&
              (smoothedGrid[pos2.y][pos2.x] == IGNORE)) {
            LOG_DEBUG("         SMOOTH1 -> " << up.t1);
            // Smooth the first (N/O) tile
            // - Need to translate the grid pos back to cave pos
            Cave::setCell(tileMap, pos1.x - 1, pos1.y - 1, up.t1);
            smoothedGrid[pos1.y][pos1.x] = SMOOTHED;
            // Check if there is a second (M) tile
            if (up.t2 != IGNORE) {
              LOG_DEBUG("      FOUND2 " << pos2.x << "," << pos2.y);
              LOG_DEBUG("         SMOOTH2 -> " << up.t2);
              // Smooth the second (M) tile
              // - Need to translate the grid pos back to cave pos
              Cave::setCell(tileMap, pos2.x - 1, pos2.y - 1, up.t2);
              smoothedGrid[pos2.y][pos2.x] = SMOOTHED;
            } else {
              LOG_DEBUG("  IGNORE TILE2: " << pos2.x << "," << pos2.y);
            }
          } else {
            LOG_DEBUG("  IGNORE p1:" << smoothedGrid[pos1.y][pos1.x]
                                     << " p2:" << smoothedGrid[pos2.y][pos2.x]);
          }
        }
        ++idx;
      }
#endif
    }
  }
}

}  // namespace Cave
