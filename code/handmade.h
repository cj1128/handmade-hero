#ifndef HANDMADE_H
#include "handmade_platform.h"
#include "handmade_intrinsic.h"

/*
  HANDMADE_INTERNAL:
    0: public build
    1: internal build

  HANDMADE_SLOW:
    1: slow code allowed
    0: no slow code
*/

#define Pi32 3.14159265359

struct tile_map {
  uint32 *Tiles;
};

struct canonical_postion {
  int32 TileMapX;
  int32 TileMapY;

  int32 TileX;
  int32 TileY;

  // TODO: Y should go up
  real32 TileRelX;
  real32 TileRelY;
};

struct game_state {
  canonical_postion PlayerPos;
};

struct world {
  int32 TileCountX;
  int32 TileCountY;

  int32 TileMapCountX;
  int32 TileMapCountY;

  real32 UpperLeftX;
  real32 UpperLeftY;

  real32 TileSizeInPixels;
  real32 TileSizeInMeters;
  real32 MetersToPixels;

  tile_map *TileMaps;
};

#define HANDMADE_H
#endif
