#ifndef HANDMADE_WORLD_H
#define HANDMADE_WORLD_H

#define TILES_PER_CHUNK 16

struct world_diff {
  v2 dXY;
};

struct stored_entity;
struct entity_block {
  stored_entity *entities[16];
  uint32 entityCount;
  entity_block *next;
};

struct world_chunk {
  int32 chunkX;
  int32 chunkY;
  int32 chunkZ;

  entity_block entityBlock;

  world_chunk *next;
};

struct world_position {
  int32 chunkX;
  int32 chunkY;
  int32 chunkZ;

  // relative to bottom left
  v2 offset_;
};

struct game_world {
  real32 tileSizeInMeters;
  real32 chunkSizeInMeters;

  entity_block *firstFree;

  // NOTE: Size has to be power of 2
  world_chunk *chunkHash[4096];
};

#endif
