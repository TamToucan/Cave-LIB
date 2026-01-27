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

//
// The TileGrid's are a 4x4 of tiles where
//   X = don't care
//   B = blank
//   S = set
//   N = loc of 1st tile to change
//   M = loc of 2nd tile to change (of loc1 a one tile update)
//   O = loc of 1st tile to change, but the tile is currently FLOOR
//
// YES. 'N'/'O' = pos1 and 'M' = pos2. Don't @ me!
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
// The smoothedGrid is a bool if tile has been smoothed
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
//
// Dead-End updates (both corners rounded)
// - East, South, West, North
// - Need 2 versions to handle map borders
//
unsigned char TileGridDEn1[GRD_H][GRD_W] = {
    {X, S, X, X}, {S, O, S, X}, {X, B, X, X}, {X, X, X, X}};
unsigned char TileGridDEn2[GRD_H][GRD_W] = {
    {X, X, S, X}, {X, S, O, S}, {X, X, B, X}, {X, X, X, X}};

unsigned char TileGridDEs1[GRD_H][GRD_W] = {
    {X, X, X, X}, {X, B, X, X}, {S, O, S, X}, {X, S, X, X}};
unsigned char TileGridDEs2[GRD_H][GRD_W] = {
    {X, X, X, X}, {X, X, B, X}, {X, S, O, S}, {X, X, S, X}};

unsigned char TileGridDEe1[GRD_H][GRD_W] = {
    {X, X, S, X}, {X, B, O, S}, {X, X, S, X}, {X, X, X, X}};
unsigned char TileGridDEe2[GRD_H][GRD_W] = {
    {X, X, X, X}, {X, X, S, X}, {X, B, O, S}, {X, X, S, X}};

unsigned char TileGridDEw1[GRD_H][GRD_W] = {
    {X, S, X, X}, {S, O, B, X}, {X, S, X, X}, {X, X, X, X}};
unsigned char TileGridDEw2[GRD_H][GRD_W] = {
    {X, X, X, X}, {X, S, X, X}, {S, O, B, X}, {X, S, X, X}};

// - N but moved and right a row to handle stop CORNER match
// and maybe problems at map corners.
// - S moved right

// - E and W but moved up a row to handle problem with border dead-ends
// getting matched with Croner updates first

//
// Corner updates (1 corner rounded)
// - Corner A, B, C, D
//
unsigned char TileGridCRa[GRD_H][GRD_W] = {
    {X, S, X, X}, {S, O, X, X}, {X, B, X, X}, {X, X, X, X}};
unsigned char TileGridCRb[GRD_H][GRD_W] = {
    {X, X, S, X}, {X, X, O, S}, {X, X, B, X}, {X, X, X, X}};
unsigned char TileGridCRc[GRD_H][GRD_W] = {
    {X, X, X, X}, {X, X, X, X}, {X, B, O, S}, {X, X, S, X}};
unsigned char TileGridCRd[GRD_H][GRD_W] = {
    {X, X, X, X}, {X, X, X, X}, {S, O, B, X}, {X, S, X, X}};

///////////////////////////////////////////

unsigned char TileDiagNE[GRD_H][GRD_W] = {
    {X, X, X, X}, {X, S, B, X}, {X, B, N, X}, {X, X, X, X}};

unsigned char TileDiagNW[GRD_H][GRD_W] = {
    {X, X, X, X}, {X, B, S, X}, {X, N, B, X}, {X, X, X, X}};

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
// For rounding anywhere there is a FLOOR with a 90 degree corner
// e.g. a dead-end of a corridor
UpdateInfo cornerUpdates[] = {
    // dead-ends (round both corners)
    {TileGridDEn1, 0, 0, 0, 0, 0, 0, DEND_N, IGNORE},
    {TileGridDEs1, 0, 0, 0, 0, 0, 0, DEND_S, IGNORE},
    {TileGridDEe1, 0, 0, 0, 0, 0, 0, DEND_E, IGNORE},
    {TileGridDEw1, 0, 0, 0, 0, 0, 0, DEND_W, IGNORE},
    {TileGridDEn2, 0, 0, 0, 0, 0, 0, DEND_N, IGNORE},
    {TileGridDEs2, 0, 0, 0, 0, 0, 0, DEND_S, IGNORE},
    {TileGridDEe2, 0, 0, 0, 0, 0, 0, DEND_E, IGNORE},
    {TileGridDEw2, 0, 0, 0, 0, 0, 0, DEND_W, IGNORE},
    // corners (round a single corner)
    {TileGridCRa, 0, 0, 0, 0, 0, 0, CORNR_A, IGNORE},
    {TileGridCRb, 0, 0, 0, 0, 0, 0, CORNR_B, IGNORE},
    {TileGridCRc, 0, 0, 0, 0, 0, 0, CORNR_C, IGNORE},
    {TileGridCRd, 0, 0, 0, 0, 0, 0, CORNR_D, IGNORE},
};

UpdateInfo diagonalUpdates[] = {
    {TileDiagNE, 0, 0, 0, 0, 0, 0, FLOOR, IGNORE},
    {TileDiagNW, 0, 0, 0, 0, 0, 0, FLOOR, IGNORE},
};

//
// For rounding anywhere there is a sharp point e.g.
// two adjacent 45 degree slopes /\.
typedef TileName TileName2x2[2][2];

struct PointUpdate {
  const TileName2x2* grids;
  int numGrids;
  int xoff1;
  int yoff1;
  TileName tile1;
};

template <size_t NUM>
constexpr PointUpdate make_point_update(const TileName2x2 (&grids)[NUM],
                                        int xoff1, int yoff1, TileName tile1) {
  return {
      grids,
      static_cast<int>(NUM),
      xoff1,
      yoff1,
      tile1};
}

static const TileName2x2 Grids_45a_2CUTS[] = {
    {{IGNORE, T45d},
     {T45b, T45a}},

    {{IGNORE, H30d2},
     {T45b, T45a}},

    {{IGNORE, T45d},
     {V60b2, T45a}},
};

static const TileName2x2 Grids_45b_2CUTS[] = {
    {{T45c, IGNORE},
     {T45b, T45a}},

    {{H30c2, IGNORE},
     {T45b, T45a}},

    {{T45c, IGNORE},
     {T45b, V60a2}},
};

static const TileName2x2 Grids_45c_2CUTS[] = {
    {{T45c, T45d},
     {T45b, IGNORE}},

    {{T45c, T45d},
     {H30b2, IGNORE}},

    {{T45c, V60d2},
     {T45b, IGNORE}},
};

static const TileName2x2 Grids_45d_2CUTS[] = {
    {{T45c, T45d},
     {IGNORE, T45a}},

    {{V60c2, T45d},
     {IGNORE, T45a}},

    {{T45c, T45d},
     {IGNORE, H30a2}},
};

// 2 corners (b and d) from T45a
static const TileName2x2 Grids_45a_bCUT[] = {
    {{IGNORE, T45d},
     {IGNORE, T45a}},

    {{IGNORE, H30d2},
     {IGNORE, T45a}},
};
static const TileName2x2 Grids_45a_dCUT[] = {
    {{IGNORE, IGNORE},
     {T45b, T45a}},

    {{IGNORE, IGNORE},
     {V60b2, T45a}},
};
// 2 corners (a and c) from T45b
static const TileName2x2 Grids_45b_aCUT[] = {
    {{T45c, IGNORE},
     {T45b, IGNORE}},

    {{H30c2, IGNORE},
     {T45b, IGNORE}},
};
static const TileName2x2 Grids_45b_cCUT[] = {
    {{IGNORE, IGNORE},
     {T45b, T45a}},

    {{IGNORE, IGNORE},
     {T45b, V60a2}},
};
// 2 corners (b and d) from T45c
static const TileName2x2 Grids_45c_bCUT[] = {
    {{T45c, T45d},
     {IGNORE, IGNORE}},

    {{T45c, V60d2},
     {IGNORE, IGNORE}},
};
static const TileName2x2 Grids_45c_dCUT[] = {
    {{T45c, IGNORE},
     {T45b, IGNORE}},

    {{T45c, IGNORE},
     {H30b2, IGNORE}},
};
// 2 corners (a and c) from T45d
static const TileName2x2 Grids_45d_aCUT[] = {
    {{T45c, T45d},
     {IGNORE, IGNORE}},

    {{V60c2, T45d},
     {IGNORE, IGNORE}},
};
static const TileName2x2 Grids_45d_cCUT[] = {
    {{IGNORE, T45d},
     {IGNORE, T45a}},

    {{IGNORE, T45d},
     {IGNORE, H30a2}},
};

static const PointUpdate Grid45a_2CUT = make_point_update(Grids_45a_2CUTS, 1, 1, T45a2CT);
static const PointUpdate Grid45b_2CUT = make_point_update(Grids_45b_2CUTS, 0, 1, T45b2CT);
static const PointUpdate Grid45c_2CUT = make_point_update(Grids_45c_2CUTS, 0, 0, T45c2CT);
static const PointUpdate Grid45d_2CUT = make_point_update(Grids_45d_2CUTS, 1, 0, T45d2CT);

static const PointUpdate Grid45a_bCUT = make_point_update(Grids_45a_bCUT, 1, 1, T45abCT);
static const PointUpdate Grid45a_dCUT = make_point_update(Grids_45a_dCUT, 1, 1, T45adCT);
static const PointUpdate Grid45b_aCUT = make_point_update(Grids_45b_aCUT, 0, 1, T45baCT);
static const PointUpdate Grid45b_cCUT = make_point_update(Grids_45b_cCUT, 0, 1, T45bcCT);
static const PointUpdate Grid45c_bCUT = make_point_update(Grids_45c_bCUT, 0, 0, T45cbCT);
static const PointUpdate Grid45c_dCUT = make_point_update(Grids_45c_dCUT, 0, 0, T45cdCT);
static const PointUpdate Grid45d_aCUT = make_point_update(Grids_45d_aCUT, 1, 0, T45daCT);
static const PointUpdate Grid45d_cCUT = make_point_update(Grids_45d_cCUT, 1, 0, T45dcCT);

static const PointUpdate pointUpdates[] = {
    Grid45a_2CUT,
    Grid45b_2CUT,
    Grid45c_2CUT,
    Grid45d_2CUT,

    Grid45a_bCUT,
    Grid45a_dCUT,
    Grid45b_aCUT,
    Grid45b_cCUT,
    Grid45c_bCUT,
    Grid45c_dCUT,
    Grid45d_aCUT,
    Grid45d_cCUT,
};

//
// Use the patterns to calc and modify the updates with mask,value and offsets
//
template <size_t SZ>
void createUpdateInfos(UpdateInfo (&updateInfos)[SZ]) {
  LOG_INFO("====================== SMOOTH CREATE UPDATES");
  for (auto& u : updateInfos) {
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
          case O:
            l_mask |= 1 << s;
            l_value |= 0 << s;
            l_xOff1 = c;
            l_yOff1 = r;
            break;
          default:
            LOG_ABORT("Invalid tile: " << grid[r][c] << " at " << r << "," << c);
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

void createUpdateInfos() {
  createUpdateInfos(updates);
  createUpdateInfos(cornerUpdates);
  createUpdateInfos(diagonalUpdates);
}

//////////////////////////////////////////////////

CaveSmoother::CaveSmoother(TileMap& tm, const CaveInfo& i)
    : info(i), tileMap(tm) {
  createUpdateInfos();
}

CaveSmoother::~CaveSmoother() {}

void CaveSmoother::smooth() {
  std::vector<std::vector<bool>> smoothedGrid(
      info.mCaveHeight + GRD_H + 1,
      std::vector<bool>(info.mCaveWidth + GRD_W + 1, false));

  if (info.mSmoothing) {
    smoothEdges(smoothedGrid);

    // Diagonls can be created by smoothing when it adds Vert/Horz tiles
    // So we smooth afterwards, however that creates blocky parts so smooth
    if (info.mRemoveDiagonals) {
      if (removeDiagonalGaps()) {
        for (auto& row : smoothedGrid) std::fill(row.begin(), row.end(), false);
        smoothEdges(smoothedGrid);
      }
    }

    if (info.mSmoothCorners) {
      smoothCorners(smoothedGrid);
    }
    if (info.mSmoothPoints) {
      smoothPoints();
    }
  } else if (info.mRemoveDiagonals) {
    removeDiagonalGaps();
  }
}

/////////////////////////////////////////////////////////////////////////////

template <size_t SZ>
bool CaveSmoother::smoothTheGrid(UpdateInfo (&updateInfos)[SZ],
                                 std::vector<std::vector<int>>& inGrid,
                                 std::vector<std::vector<bool>>& smoothedGrid,
                                 bool updateInGrid) {
  bool changed = false;
  //
  // Smooth the grid
  //
  for (int y = 0; y < info.mCaveHeight; y++) {
    for (int x = 0; x < info.mCaveWidth; x++) {
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
      LOG_DEBUG("==FIND " << x << "," << y << " val:" << std::hex << value << std::dec);

      // Find the matching update(s) for that value
      //
      int idx = 0;
      for (const auto& up : updateInfos) {
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
          if ((smoothedGrid[pos1.y][pos1.x] == false) &&
              (smoothedGrid[pos2.y][pos2.x] == false)) {
            LOG_DEBUG("         SMOOTH1 -> " << up.t1);
            // Smooth the first (N/O) tile
            // - Need to translate the grid pos back to cave pos
            Cave::setCell(tileMap, pos1.x - 1, pos1.y - 1, up.t1);
            // Removing Diagonals needs to update the inGrid
            if (updateInGrid) {
              inGrid[pos1.y][pos1.x] = up.t1;
            }
            smoothedGrid[pos1.y][pos1.x] = true;
            changed = true;
            // Check if there is a second (M) tile
            if (up.t2 != IGNORE) {
              LOG_DEBUG("      FOUND2 " << pos2.x << "," << pos2.y);
              LOG_DEBUG("         SMOOTH2 -> " << up.t2);
              // Smooth the second (M) tile
              // - Need to translate the grid pos back to cave pos
              Cave::setCell(tileMap, pos2.x - 1, pos2.y - 1, up.t2);
              // Removing Diagonals needs to update the inGrid
              if (updateInGrid) {
                inGrid[pos2.y][pos2.x] = up.t2;
              }
              smoothedGrid[pos2.y][pos2.x] = true;
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
    }
  }
  return changed;
}

/////////////////////////////////////////////////////////////////////////////

//
// Copy the input, create a 4x4 grid value for each pos (bit set = wall)
// and find any matching update(s). For each match set the TileMapLayer
// cell(s) for the 1 or 2 tiles for each update.
//
void CaveSmoother::smoothEdges(std::vector<std::vector<bool>>& smoothedGrid) {
  //
  // NOTE: So we can do a 4x4 with the top and left edge being the border
  // we shift the maze 0,0 to 1,1. We also make it wider to allow the
  // right and bottom edges to be a border
  //
  LOG_INFO("====================== SMOOTH EDGES");
  //
  // Copy the current cave
  // NOTE: Translate the cave 0,0 => 1,1 of grids
  //
  std::vector<std::vector<int>> inGrid(
      info.mCaveHeight + GRD_H + 1,
      std::vector<int>(info.mCaveWidth + GRD_W + 1, SOLID));

  for (int y = 0; y < info.mCaveHeight; y++) {
    for (int x = 0; x < info.mCaveWidth; x++) {
      inGrid[y + 1][x + 1] = Cave::isWall(tileMap, x, y) ? SOLID : FLOOR;
    }
  }
  smoothTheGrid(updates, inGrid, smoothedGrid);
}

void CaveSmoother::smoothCorners(std::vector<std::vector<bool>>& smoothedGrid) {
  //
  // NOTE: So we can do a 4x4 with the top and left edge being the border
  // we shift the maze 0,0 to 1,1. We also make it wider to allow the
  // right and bottom edges to be a border
  //
  LOG_INFO("====================== SMOOTH CORNERS");
  //
  // Copy the current cave
  // NOTE: Translate the cave 0,0 => 1,1 of grids
  //
  std::vector<std::vector<int>> inGrid(
      info.mCaveHeight + GRD_H + 1,
      std::vector<int>(info.mCaveWidth + GRD_W + 1, SOLID));

  for (int y = 0; y < info.mCaveHeight; y++) {
    for (int x = 0; x < info.mCaveWidth; x++) {
      // Walls and End caps can make right angle corners we want to round
      // Thought I could do something clever with IGNORE vs FLOOR, but all
      // the smoothed tiles are treated as not set, hence I pass in the smoothedGrid
      bool isWall = Cave::isWall(tileMap, x, y) || Cave::isTile(tileMap, x, y, END_N) || Cave::isTile(tileMap, x, y, END_S) || Cave::isTile(tileMap, x, y, END_E) || Cave::isTile(tileMap, x, y, END_W);
      inGrid[y + 1][x + 1] = isWall                         ? SOLID
                             : Cave::isFloor(tileMap, x, y) ? FLOOR
                                                            : IGNORE;
    }
  }
  smoothTheGrid(cornerUpdates, inGrid, smoothedGrid);
}

void CaveSmoother::smoothPoints() {
  LOG_INFO("====================== SMOOTH POINTS");
  auto tileMapCopy(tileMap);
  std::vector<std::vector<bool>> smoothedGrid(
      info.mCaveHeight + 2 + 1,
      std::vector<bool>(info.mCaveWidth + 2 + 1, false));
  for (int y = 0; y < info.mCaveHeight; y++) {
    for (int x = 0; x < info.mCaveWidth; x++) {
      int idx = 0;
      for (const auto& up : pointUpdates) {
        for (int i = 0; i < up.numGrids; ++i) {
          if (smoothedGrid[y + up.yoff1][x + up.xoff1]) continue;
          bool match = true;
          LOG_DEBUG("SPNT: " << x << "," << y << " up:" << up.xoff1 << "," << up.yoff1 << " tile:" << up.tile1);
          const auto* grid = up.grids[i];
          for (int yo = 0; yo < 2 && match; ++yo) {
            for (int xo = 0; xo < 2 && match; ++xo) {
              TileName wantTile = grid[yo][xo];
              if (wantTile != IGNORE) {
                // Use the original grid to check for matches
                if (!Cave::isTile(tileMapCopy, x + xo, y + yo, wantTile)) {
                  match = false;
                } else {
                  LOG_DEBUG("...match off: " << xo << "," << yo);
                }
              }
            }
          }
          if (match) {
            LOG_DEBUG("...FULL MATCH set:" << x + 1 + up.xoff1 << "," << y + 1 + up.yoff1
                                           << " tile:" << up.tile1);
            Cave::setCell(tileMap, x + up.xoff1, y + up.yoff1, up.tile1);
            smoothedGrid[y + up.yoff1][x + up.xoff1] = true;
            break;
          }
        }
      }
    }
  }
}

bool CaveSmoother::removeDiagonalGaps() {
  std::vector<std::vector<bool>> smoothedGrid(
      info.mCaveHeight + GRD_H + 1,
      std::vector<bool>(info.mCaveWidth + GRD_W + 1, false));

  //
  // NOTE: So we can do a 4x4 with the top and left edge being the border
  // we shift the maze 0,0 to 1,1. We also make it wider to allow the
  // right and bottom edges to be a border
  //
  LOG_INFO("====================== REMOVE DIAGONAL GAPS");
  //
  // Copy the current cave
  // NOTE: Translate the cave 0,0 => 1,1 of grids
  //
  std::vector<std::vector<int>> inGrid(
      info.mCaveHeight + GRD_H + 1,
      std::vector<int>(info.mCaveWidth + GRD_W + 1, SOLID));

  for (int y = 0; y < info.mCaveHeight; y++) {
    for (int x = 0; x < info.mCaveWidth; x++) {
      inGrid[y + 1][x + 1] = Cave::isEmpty(tileMap, x, y) ? FLOOR : SOLID;
    }
  }
  return smoothTheGrid(diagonalUpdates, inGrid, smoothedGrid, true);
}

}  // namespace Cave