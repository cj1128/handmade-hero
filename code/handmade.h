#ifndef HANDMADE_H
#define HANDMADE_H

#include "handmade_platform.h"
#include "handmade_intrinsic.h"
#include "handmade_math.h"

#define PI32 3.14159265359

struct memory_arena {
  size_t size;
  uint8 *base;
  size_t used;
};

internal void *
PushSize_(memory_arena *arena, size_t size) {
  Assert((arena->used + size) <= arena->size);
  uint8 *result = arena->base + arena->used;
  arena->used += size;
  return (void *)result;
}

#define PushArray(arena, count, type) (type *)PushSize_(arena, (count)*sizeof(type))
#define PushStruct(arena, type) (type *)PushSize_((arena), sizeof(type))

struct loaded_bitmap {
  uint32 *pixel;
  int32 width;
  int32 height;
};

struct render_piece {
  loaded_bitmap *bitmap;
  v2 halfDim;
  v3 color;
  v2 offset;
};

struct render_piece_group {
  uint32 pieceCount;
  render_piece pieces[16];
};

struct hero_bitmaps {
  v2 offset;
  loaded_bitmap head;
  loaded_bitmap cape;
  loaded_bitmap torso;
};

enum entity_type {
  EntityType_Hero,
  EntityType_Wall,
  EntityType_Monster,
  EntityType_Familiar,
  EntityType_Sword,
};

#define HIT_POINT_AMOUNT 4
struct hit_point {
  uint8 flags;
  uint8 amount;
};

#include "handmade_world.cpp"

struct high_entity;
struct low_entity {
  entity_type type;

  world_position p;

  real32 width;

  real32 height;

  bool32 collides;

  int32 dAbsTileZ;

  high_entity *highEntity;

  uint32 hitPointCount;

  hit_point hitPoints[16];

  low_entity *sword;

  real32 distanceRemaining;
};

struct high_entity {
  v2 p;
  v2 dP;
  int32 chunkZ;
  // 0: right, 1: up, 2: left, 3: down
  uint32 facingDirection;
  low_entity *lowEntity;
};


struct game_state {
  memory_arena memoryArena;
  game_world world;
  world_position cameraP;

  uint32 highEntityCount;
  high_entity highEntities[512];

  uint32 lowEntityCount;
  low_entity lowEntities[4096];

  low_entity *playerEntityForController[ArrayCount(((game_input *)0)->controllers)];

  low_entity *cameraFollowingEntity;

  loaded_bitmap background;
  loaded_bitmap tree;
  loaded_bitmap sword;
  hero_bitmaps heroBitmaps[4];
};

#endif
