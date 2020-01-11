#ifndef HANDMADE_WORLD_H
#define TILES_PER_CHUNK 16

struct world_diff {
  v2 dXY;
};

struct low_entity;
struct entity_block {
  low_entity *LowEntities[16];
  uint32 EntityCount;
  entity_block *Next;
};

struct world_chunk {
  int32 ChunkX;
  int32 ChunkY;
  int32 ChunkZ;

  entity_block EntityBlock;

  world_chunk *Next;
};

struct world_position {
  int32 ChunkX;
  int32 ChunkY;
  int32 ChunkZ;

  // relative to bottom left
  v2 Offset_;
};

struct world {
  real32 TileSizeInMeters;
  real32 ChunkSizeInMeters;

  entity_block *FirstFree;

  // NOTE: Currently this has to be power of 2
  world_chunk *ChunkHash[4096];
};

#define HANDMADE_WORLD_H value
#endif
