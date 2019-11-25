#ifndef HANDMADE_TILE_H

struct tile_chunk {
  uint32 *Tiles;
};

struct tile_chunk_position {
  uint32 TileChunkX;
  uint32 TileChunkY;
  uint32 RelTileX;
  uint32 RelTileY;
};

struct tile_map {
  uint32 TileChunkShift;
  uint32 TileChunkMask;
  uint32 TileChunkDim;

  uint32 TileChunkCountX;
  uint32 TileChunkCountY;

  real32 TileSizeInPixels;
  real32 TileSizeInMeters;
  real32 MetersToPixels;

  tile_chunk *TileChunks;
};

struct tile_map_position {
  uint32 AbsTileX;
  uint32 AbsTileY;

  // relative to central of tile
  real32 TileRelX;
  real32 TileRelY;
};

#define HANDMADE_TILE_H
#endif
