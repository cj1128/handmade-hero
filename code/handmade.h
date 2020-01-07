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

#define PushArray(Arena, Count, Type) (Type *)PushSize_(Arena, (Count)*sizeof(Type))
#define PushStruct(Arena, Type) (Type *)PushSize_((Arena), sizeof(Type))

#include "handmade_math.h"
#include "handmade_world.cpp"

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

enum entity_type {
  EntityType_Hero,
  EntityType_Wall,
};

struct low_entity;

struct high_entity {
  v2 P;
  v2 dP;
  int32 AbsTileZ;
  // 0: right, 1: up, 2: left, 3: down
  uint32 FacingDirection;
  low_entity *LowEntity;
};

struct low_entity {
  entity_type Type;
  world_position P;
  real32 Width;
  real32 Height;
  bool32 Collides;
  int32 dAbsTileZ;
  high_entity *HighEntity;
};

struct game_state {
  memory_arena MemoryArena;
  world World;
  world_position CameraP;

  uint32 HighEntityCount;
  high_entity HighEntities[512];

  uint32 LowEntityCount;
  low_entity LowEntities[4096];

  low_entity *PlayerEntityForController[ArrayCount(((game_input *)0)->Controllers)];

  low_entity *CameraFollowingEntity;

  loaded_bitmap Background;
  hero_bitmaps HeroBitmaps[4];
};

#define HANDMADE_H
#endif
