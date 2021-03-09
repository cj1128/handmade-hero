#ifndef HANDMADE_WORLD_H
#define HANDMADE_WORLD_H

#define TILES_PER_CHUNK 8

struct stored_entity;
struct entity_block {
  stored_entity *entities[16];
  u32 entityCount;
  entity_block *next;
};

struct world_chunk {
  i32 chunkX;
  i32 chunkY;
  i32 chunkZ;

  entity_block entityBlock;

  world_chunk *next;
};

struct world_position {
  i32 chunkX;
  i32 chunkY;
  i32 chunkZ;

  // relative to the center
  v3 offset;
};

struct game_world {
  f32 typicalFloorHeight;
  v3 chunkDimInMeters;

  entity_block *firstFree;

  // NOTE(cj): size needs to be power of 2
  world_chunk *chunkHash[4096];
};

#endif
