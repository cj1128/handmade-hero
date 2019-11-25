#ifndef HANDMADE_H
#include "handmade_platform.h"
#include "handmade_intrinsic.h"
#include "handmade_tile.h"
#include "handmade_tile.cpp"

/*
  HANDMADE_INTERNAL:
    0: public build
    1: internal build

  HANDMADE_SLOW:
    1: slow code allowed
    0: no slow code
*/

#define Pi32 3.14159265359

struct memory_arena {
  size_t Size;
  uint8 *Base;
  size_t Used;
};

struct world {
  tile_map TileMap;
};

struct game_state {
  memory_arena MemoryArena;
  world World;
  tile_map_position PlayerPos;
};

#define HANDMADE_H
#endif
