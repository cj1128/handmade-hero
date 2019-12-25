#ifndef HANDMADE_H
#include "handmade_platform.h"
#include "handmade_intrinsic.h"

#define Pi32 3.14159265359

struct memory_arena {
  size_t Size;
  uint8 *Base;
  size_t Used;
};

internal void *
PushSize_(memory_arena *Arena, size_t Size) {
  Assert((Arena->Used + Size) <= Arena->Size);
  uint8 *Result = Arena->Base + Arena->Used;
  Arena->Used += Size;
  return (void *)Result;
}

#define PushArray(Arena, Count, Type) (Type *)PushArray_((Arena), (Count), sizeof(Type))

internal void *
PushArray_(memory_arena *Arena, uint32 Count, size_t Size) {
  return PushSize_(Arena, Count*Size);
}

#include "handmade_math.h"
#include "handmade_tile.cpp"

struct world {
  tile_map TileMap;
};

struct loaded_bitmap {
  uint32 *Pixel;
  int32 Width;
  int32 Height;
};

struct hero_bitmaps {
  real32 OffsetX;
  real32 OffsetY;

  loaded_bitmap Head;
  loaded_bitmap Cape;
  loaded_bitmap Torso;
};

struct entity {
  bool32 Exists;
  tile_map_position P;
  v2 dP;
  // 0: right, 1: up, 2: left, 3: down
  uint32 FacingDirection;
  real32 Width;
  real32 Height;
};

struct game_state {
  memory_arena MemoryArena;
  world World;
  tile_map_position CameraP;

  uint32 EntityCount;
  entity Entities[256];
  uint32 PlayerIndexForController[ArrayCount(((game_input *)0)->Controllers)];

  uint32 CameraFollowingEntityIndex;

  loaded_bitmap Background;
  hero_bitmaps HeroBitmaps[4];
};

#define HANDMADE_H
#endif
