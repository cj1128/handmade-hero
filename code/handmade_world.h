#ifndef HANDMADE_WORLD_H

struct world_diff {
  v2 dXY;
};

struct world_chunk {
  int32 ChunkX;
  int32 ChunkY;
  int32 AbsTileZ;

  world_chunk *NextInHash;
};

struct world_position {
  int32 AbsTileX;
  int32 AbsTileY;
  int32 AbsTileZ;

  // relative to tile center
  v2 Offset_;
};

struct world {
  uint32 ChunkShift;
  uint32 ChunkMask;
  uint32 ChunkDim;

  real32 TileSizeInMeters;

  // NOTE: Currently this has to be power of 2
  world_chunk *ChunkHash[4096];
};

#define HANDMADE_WORLD_H value
#endif
