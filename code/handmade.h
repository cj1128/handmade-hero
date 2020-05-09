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
PushSize_(memory_arena *arena, size_t size)
{
  Assert((arena->used + size) <= arena->size);
  uint8 *result = arena->base + arena->used;
  arena->used += size;
  return (void *)result;
}

internal void
ZeroSize(void *ptr, size_t size)
{
  uint8 *byte = (uint8 *)ptr;
  while(size--) {
    *byte++ = 0;
  }
}

#define PushArray(arena, count, type)                                          \
  (type *)PushSize_(arena, (count) * sizeof(type))
#define PushStruct(arena, type) (type *)PushSize_((arena), sizeof(type))

#define ZeroStruct(instance) ZeroSize(&(instance), sizeof(instance))

struct loaded_bitmap {
  uint32 *pixel;
  int32 width;
  int32 height;
};

struct render_piece {
  loaded_bitmap *bitmap;
  v2 halfDim;
  v3 color;
  real32 alpha;
  v2 offset;
  real32 entityZC;
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

#include "handmade_world.h"
#include "handmade_entity.h"
#include "handmade_sim_region.h"

struct controlled_hero {
  stored_entity *stored;

  v2 ddP;
  v3 dSword;
  real32 dZ;
};

struct pairwise_collision_rule {
  stored_entity *a;
  stored_entity *b;
  bool32 shouldCollide;
  pairwise_collision_rule *nextInHash;
};

struct game_state {
  memory_arena worldArena;
  game_world world;
  world_position cameraP;

  bool32 debugDrawBoundary = false;

  uint32 entityCount;
  stored_entity entities[4096];

  controlled_hero controlledHeroes[ArrayCount(((game_input *)0)->controllers)];

  stored_entity *cameraFollowingEntity;

  loaded_bitmap background;
  loaded_bitmap tree;
  loaded_bitmap sword;
  loaded_bitmap shadow;
  hero_bitmaps heroBitmaps[4];

  // NOTE: need to be power of two
  pairwise_collision_rule *collisionRuleHash[4096];
  pairwise_collision_rule *firstFreeCollisionRule;
};

#endif
