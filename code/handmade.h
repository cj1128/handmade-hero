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

struct tile_map_position {
  uint32 TileMapX;
  uint32 TileMapY;
  uint32 RelTileX;
  uint32 RelTileY;
};

struct world_position {
  uint32 AbsTileX;
  uint32 AbsTileY;

  // NOTE: relative to lower left corner
  real32 TileRelX;
  real32 TileRelY;
};

struct game_state {
  world_position PlayerPos;
};

struct world {
  uint32 TileMapShift;
  uint32 TileMapMask;
  uint32 TileMapDim;

  int32 TileCountX;
  int32 TileCountY;

  uint32 TileMapCountX;
  uint32 TileMapCountY;

  real32 TileSizeInPixels;
  real32 TileSizeInMeters;
  real32 MetersToPixels;

  tile_map *TileMaps;
};

#define HANDMADE_H
#endif
