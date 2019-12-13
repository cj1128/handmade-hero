#ifndef HANDMADE_TILE_H

struct tile_map_diff {
  v2 dXY;
};

struct tile_chunk {
  uint32 *Tiles;
};

struct tile_chunk_position {
  uint32 TileChunkX;
  uint32 TileChunkY;
  uint32 TileChunkZ;
  uint32 RelTileX;
  uint32 RelTileY;
};

struct tile_map {
  uint32 TileChunkShift;
  uint32 TileChunkMask;
  uint32 TileChunkDim;

  uint32 TileChunkCountX;
  uint32 TileChunkCountY;
  uint32 TileChunkCountZ;

  real32 TileSizeInMeters;

  tile_chunk *TileChunks;
};

struct tile_map_position {
  uint32 AbsTileX;
  uint32 AbsTileY;
  uint32 AbsTileZ;

  // relative to tile center
  v2 Offset;
};

#define HANDMADE_TILE_H
#endif
