#ifndef HANDMADE_TILE_H
#define HANDMADE_TILE_H

struct tile_map_difference
{
    v2 dXY;
    real32 dZ;
};

struct tile_map_pos
{
  uint32 AbsTileX;
  uint32 AbsTileY;
  uint32 AbsTileZ;

  v2 Offset;
};

struct tile_chunk
{
  uint32 *Tiles;
};

struct tile_map
{
  tile_chunk *TileChunks;

  uint32 ChunkShift;
  uint32 ChunkMask;
  uint32 ChunkDim;

  real32 TileSideInMeters;

  uint32 TileChunkCountX;
  uint32 TileChunkCountY;
  uint32 TileChunkCountZ;
};

#endif
