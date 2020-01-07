#ifndef HANDMADE_TILE_H

struct tile_map_diff {
  v2 dXY;
};

struct tile_chunk {
  uint32 *Tiles;
  int32 TileChunkX;
  int32 TileChunkY;
  int32 AbsTileZ;

  tile_chunk *NextInHash;
};

struct tile_chunk_position {
  int32 TileChunkX;
  int32 TileChunkY;
  int32 AbsTileZ;

  int32 RelTileX;
  int32 RelTileY;
};

struct tile_map {
  uint32 TileChunkShift;
  uint32 TileChunkMask;
  uint32 TileChunkDim;

  real32 TileSizeInMeters;

  // NOTE: Currently this has to be power of 2
  tile_chunk *TileChunkHash[4096];
};

struct tile_map_position {
  int32 AbsTileX;
  int32 AbsTileY;
  int32 AbsTileZ;

  // relative to tile center
  v2 Offset_;
};

#define HANDMADE_TILE_H
#endif
