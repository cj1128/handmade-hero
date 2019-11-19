#ifndef HANDMADE_H
#include "handmade_platform.h"

/*
  HANDMADE_INTERNAL:
    0: public build
    1: internal build

  HANDMADE_SLOW:
    1: slow code allowed
    0: no slow code
*/

#define Pi32 3.14159265359

struct game_state {
  real32 PlayerX;
  real32 PlayerY;
  int32 TileMapX;
  int32 TileMapY;
};

struct tile_map {
  uint32 *Tiles;
};

struct canonical_postion {
  int32 TileMapX;
  int32 TileMapY;

  int32 TileX;
  int32 TileY;

  real32 TileRelX;
  real32 TileRelY;
};

struct raw_position {
  int32 TileMapX;
  int32 TileMapY;

  real32 X;
  real32 Y;
};

struct world {
  int32 TileCountX;
  int32 TileCountY;

  int32 TileMapCountX;
  int32 TileMapCountY;

  real32 UpperLeftX;
  real32 UpperLeftY;

  real32 TileSize;
  tile_map *TileMaps;
};

#define HANDMADE_H
#endif
