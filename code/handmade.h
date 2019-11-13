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
};

struct tile_map {
  int32 CountX;
  int32 CountY;
  real32 Top;
  real32 Left;
  real32 TileSize;
  uint32 *Tiles;
};

#define HANDMADE_H
#endif
