internal inline tile_chunk_position
GetTileChunkPosition(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY) {
  tile_chunk_position Result = {};

  Result.TileChunkX = AbsTileX >> TileMap->TileChunkShift;
  Result.RelTileX = AbsTileX & TileMap->TileChunkMask;

  Result.TileChunkY = AbsTileY >> TileMap->TileChunkShift;
  Result.RelTileY = AbsTileY & TileMap->TileChunkMask;

  return Result;
}

inline tile_chunk *
GetTileChunk(tile_map *TileMap, uint32 TileChunkX, uint32 TileChunkY) {
  tile_chunk *Result = NULL;

  if(TileChunkX < TileMap->TileChunkCountX && TileChunkY < TileMap->TileChunkCountY) {
    Result = &TileMap->TileChunks[TileChunkY * TileMap->TileChunkCountX + TileChunkX];
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
GetTileValue(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY) {
  uint32 Result = 2;

  tile_chunk_position P = GetTileChunkPosition(TileMap, AbsTileX, AbsTileY);
  tile_chunk *TileChunk = GetTileChunk(TileMap, P.TileChunkX, P.TileChunkY);

  if(TileChunk) {
    Result = TileChunk->Tiles[P.RelTileY * TileMap->TileChunkDim + P.RelTileX];
  }

  return Result;
}

internal inline tile_map_position
RecononicalizePosition(tile_map *TileMap, tile_map_position Pos) {
  tile_map_position Result = Pos;
  RecononicalizeCoord(TileMap, &Result.AbsTileX, &Result.TileRelX);
  RecononicalizeCoord(TileMap, &Result.AbsTileY, &Result.TileRelY);
  return Result;
}

bool32
IsTileMapEmtpy(tile_map *TileMap, tile_map_position Pos) {
  bool32 Empty = false;

  Empty = GetTileValue(TileMap, Pos.AbsTileX, Pos.AbsTileY) == 0;

  return Empty;
}

inline void
SetTileValue(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 TileValue) {
  tile_chunk_position P = GetTileChunkPosition(TileMap, AbsTileX, AbsTileY);
  tile_chunk *TileChunk = GetTileChunk(TileMap, P.TileChunkX, P.TileChunkY);

  Assert(TileChunk);

  TileChunk->Tiles[P.RelTileY * TileMap->TileChunkDim + P.RelTileX] = TileValue;
}
