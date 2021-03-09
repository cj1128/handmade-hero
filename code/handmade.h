#ifndef HANDMADE_H
#define HANDMADE_H

#include "handmade_platform.h"
#include "handmade_intrinsic.h"
#include "handmade_math.h"

#define PI32 3.14159265359

struct memory_arena {
  size_t size;
  u8 *base;
  size_t used;
  u32 _savedCount;
  size_t _saved[8];
};

internal void
SaveArena(memory_arena *arena)
{
  Assert(arena->_savedCount < ArrayCount(arena->_saved));
  arena->_saved[arena->_savedCount++] = arena->used;
}

internal void
RestoreArena(memory_arena *arena)
{
  Assert(arena->_savedCount > 0);
  arena->used = arena->_saved[--arena->_savedCount];
}

internal void
CheckArena(memory_arena *arena)
{
  Assert(arena->_savedCount == 0);
}

internal void *
_PushSize(memory_arena *arena, size_t size)
{
  Assert((arena->used + size) <= arena->size);
  u8 *result = arena->base + arena->used;
  arena->used += size;
  return (void *)result;
}

internal void
ZeroSize(void *ptr, size_t size)
{
  u8 *byte = (u8 *)ptr;
  while(size--) {
    *byte++ = 0;
  }
}

#define PushArray(arena, count, type)                                          \
  (type *)PushSize(arena, (count) * sizeof(type))
#define PushStruct(arena, type) (type *)PushSize((arena), sizeof(type))
#define PushSize(arena, size) _PushSize((arena), (size))

#define ZeroStruct(instance) ZeroSize(&(instance), sizeof(instance))

struct loaded_bitmap {
  void *memory;
  i32 width;
  i32 height;
  i32 pitch;
};

struct hero_bitmaps {
  v2 align;
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
  f32 dZ;
};

struct pairwise_collision_rule {
  stored_entity *a;
  stored_entity *b;
  bool32 canCollide;
  pairwise_collision_rule *nextInHash;
};

struct ground_buffer {
  world_position p; // center of the bitmap
  loaded_bitmap bitmap;
  // void *memory;
};

struct transient_state {
  bool32 isInitialized;
  memory_arena tranArena;
  i32 groundWidth;
  i32 groundHeight;
  i32 groundPitch;
  u32 groundBufferCount;
  ground_buffer *groundBuffers;
};

struct game_state {
  f32 metersToPixels;
  f32 pixelsToMeters;
  memory_arena worldArena;
  game_world world;
  world_position cameraP;

  bool32 debugDrawBoundary = false;

  u32 entityCount;
  stored_entity entities[4096];

  controlled_hero controlledHeroes[ArrayCount(((game_input *)0)->controllers)];

  stored_entity *cameraFollowingEntity;

  loaded_bitmap grass[2];
  loaded_bitmap ground[4];
  loaded_bitmap tuft[3];

  loaded_bitmap background;
  loaded_bitmap tree;
  loaded_bitmap sword;
  loaded_bitmap shadow;
  hero_bitmaps heroBitmaps[4];

  // NOTE: need to be power of two
  pairwise_collision_rule *collisionRuleHash[4096];
  pairwise_collision_rule *firstFreeCollisionRule;

  sim_entity_collision_volume_group *wallCollision;
  sim_entity_collision_volume_group *standardRoomCollision;
  sim_entity_collision_volume_group *swordCollision;
  sim_entity_collision_volume_group *heroCollision;
  sim_entity_collision_volume_group *familiarCollision;
  sim_entity_collision_volume_group *monsterCollision;
  sim_entity_collision_volume_group *stairCollision;
  sim_entity_collision_volume_group *nullCollision;
};

#endif
