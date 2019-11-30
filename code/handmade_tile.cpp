internal inline tile_chunk_position
GetTileChunkPosition(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  tile_chunk_position Result = {};

  Result.TileChunkX = AbsTileX >> TileMap->TileChunkShift;
  Result.RelTileX = AbsTileX & TileMap->TileChunkMask;

  Result.TileChunkY = AbsTileY >> TileMap->TileChunkShift;
  Result.RelTileY = AbsTileY & TileMap->TileChunkMask;

  Result.TileChunkZ = AbsTileZ;

  return Result;
}

inline tile_chunk *
GetTileChunk(tile_map *TileMap, uint32 TileChunkX, uint32 TileChunkY, uint32 TileChunkZ) {
  tile_chunk *Result = NULL;

  if(TileChunkX < TileMap->TileChunkCountX && TileChunkY < TileMap->TileChunkCountY &&
    TileChunkZ < TileMap->TileChunkCountZ) {
    Result = &TileMap->TileChunks[TileChunkZ*TileMap->TileChunkCountY*TileMap->TileChunkCountX + TileChunkY*TileMap->TileChunkCountX + TileChunkX];
  }

  return Result;
}

internal inline void
RecononicalizeCoord(tile_map *TileMap, uint32 *Tile, real32 *TileRel) {
  // NOTE: tile map is assumed to be toroidal
  int32 Offset = RoundReal32ToInt32(*TileRel / TileMap->TileSizeInMeters);
  *Tile += Offset;
  *TileRel -= Offset * TileMap->TileSizeInMeters;

  Assert(*TileRel >= -0.5f*TileMap->TileSizeInMeters);
  Assert(*TileRel < 0.5f*TileMap->TileSizeInMeters);
}

inline uint32
GetTileValue(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  uint32 Result = 0;

  tile_chunk_position P = GetTileChunkPosition(TileMap, AbsTileX, AbsTileY, AbsTileZ);
  tile_chunk *TileChunk = GetTileChunk(TileMap, P.TileChunkX, P.TileChunkY, P.TileChunkZ);

  if(TileChunk && TileChunk->Tiles) {
    Result = TileChunk->Tiles[P.RelTileY * TileMap->TileChunkDim + P.RelTileX];
  }

  return Result;
}

internal inline tile_map_position
RecononicalizePosition(tile_map *TileMap, tile_map_position Pos) {
  tile_map_position Result = Pos;
  RecononicalizeCoord(TileMap, &Result.AbsTileX, &Result.OffsetX);
  RecononicalizeCoord(TileMap, &Result.AbsTileY, &Result.OffsetY);
  return Result;
}

bool32
IsTileMapEmtpy(tile_map *TileMap, tile_map_position Pos) {
  bool32 Empty = false;

  uint32 TileValue = GetTileValue(TileMap, Pos.AbsTileX, Pos.AbsTileY, Pos.AbsTileZ);
  Empty = (TileValue == 1) || (TileValue == 3);

  return Empty;
}

inline void
SetTileValue(memory_arena *Arena, tile_map *TileMap,
  uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ,
  uint32 TileValue
) {
  tile_chunk_position P = GetTileChunkPosition(TileMap, AbsTileX, AbsTileY, AbsTileZ);
  tile_chunk *TileChunk = GetTileChunk(TileMap, P.TileChunkX, P.TileChunkY, P.TileChunkZ);

  Assert(TileChunk);
  if(!TileChunk->Tiles) {
    uint32 TileCount = TileMap->TileChunkDim*TileMap->TileChunkDim;
    TileChunk->Tiles = PushArray(Arena, TileCount, uint32);
    for(uint32 TileIndex = 0; TileIndex < TileCount; TileIndex++) {
      TileChunk->Tiles[TileIndex] = 1;
    }
  }

  TileChunk->Tiles[P.RelTileY * TileMap->TileChunkDim + P.RelTileX] = TileValue;
}

inline bool32
AreSameTiles(tile_map_position P1, tile_map_position P2) {
  return P1.AbsTileX == P2.AbsTileX &&
    P1.AbsTileY == P2.AbsTileY &&
    P1.AbsTileZ == P2.AbsTileZ;
}

tile_map_diff
SubtractPosition(tile_map *TileMap, tile_map_position P1, tile_map_position P2) {
  tile_map_diff Result = {};
  Result.dX = TileMap->TileSizeInMeters * ((real32)P1.AbsTileX - (real32)P2.AbsTileX) + P1.OffsetX - P2.OffsetX;
  Result.dY = TileMap->TileSizeInMeters * ((real32)P1.AbsTileY - (real32)P2.AbsTileY) + P1.OffsetY - P2.OffsetY;
  return Result;
}
