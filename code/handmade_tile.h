#ifndef HANDMADE_TILE_H
#define HANDMADE_TILE_H

struct tile_map_pos
{
  uint32 AbsTileX;
  uint32 AbsTileY;

  real32 TileRelX;
  real32 TileRelY;
};

struct tile_chunk
{
  uint32 *Tiles;
};

struct tile_map
{
  tile_chunk *TileChunks;

  int32 TileSideInPixels;
  real32 TileSideInMeters;
  real32 MetersToPixels;

  uint32 ChunkShift;
  uint32 ChunkMask;
  uint32 ChunkDim;

  uint32 TileChunkCountX;
  uint32 TileChunkCountY;
};

struct world
{
    tile_map *TileMap;
};


#endif