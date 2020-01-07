#include "handmade_tile.h"

inline tile_chunk_position
GetTileChunkPosition(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  tile_chunk_position Result = {};

  Result.TileChunkX = AbsTileX >> TileMap->TileChunkShift;
  Result.RelTileX = AbsTileX & TileMap->TileChunkMask;

  Result.TileChunkY = AbsTileY >> TileMap->TileChunkShift;
  Result.RelTileY = AbsTileY & TileMap->TileChunkMask;

  Result.AbsTileZ = AbsTileZ;

  return Result;
}

inline tile_chunk *
GetTileChunk(tile_map *TileMap, int32 TileChunkX, int32 TileChunkY, int32 AbsTileZ, memory_arena *Arena=NULL) {
  uint32 HashIndex = (TileChunkX*19 + TileChunkY*7 + AbsTileZ*3) & (ArrayCount(TileMap->TileChunkHash) - 1);
  tile_chunk *Chunk = TileMap->TileChunkHash[HashIndex];

  while(Chunk) {
    if(Chunk->TileChunkX == TileChunkX &&
      Chunk->TileChunkY == TileChunkY &&
      Chunk->AbsTileZ == AbsTileZ
    ) {
      break;
    }

    Chunk = Chunk->NextInHash;
  }

  if(Chunk == NULL && Arena) {
    tile_chunk *NewChunk = PushStruct(Arena, tile_chunk);
    NewChunk->TileChunkX = TileChunkX;
    NewChunk->TileChunkY = TileChunkY;
    NewChunk->AbsTileZ = AbsTileZ;
    uint32 TileCount = TileMap->TileChunkDim*TileMap->TileChunkDim;
    NewChunk->Tiles = PushArray(Arena, TileCount, uint32);

    for(uint32 Index = 0; Index < TileCount; Index++) {
      NewChunk->Tiles[Index] = 1;
    }

    NewChunk->NextInHash = TileMap->TileChunkHash[HashIndex];
    TileMap->TileChunkHash[HashIndex] = NewChunk;

    return NewChunk;
  }

  return Chunk;
}

internal inline void
RecanonicalizeCoord(tile_map *TileMap, int32 *Tile, real32 *TileRel) {
  // NOTE: tile map is assumed to be toroidal
  int32 Offset = RoundReal32ToInt32(*TileRel / TileMap->TileSizeInMeters);
  *Tile += Offset;
  *TileRel -= Offset * TileMap->TileSizeInMeters;

  Assert(*TileRel >= -0.50001f*TileMap->TileSizeInMeters);
  Assert(*TileRel < 0.50001f*TileMap->TileSizeInMeters);
}

inline uint32
GetTileValue(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  uint32 Result = 0;

  tile_chunk_position P = GetTileChunkPosition(TileMap, AbsTileX, AbsTileY, AbsTileZ);
  tile_chunk *TileChunk = GetTileChunk(TileMap, P.TileChunkX, P.TileChunkY, P.AbsTileZ);

  if(TileChunk && TileChunk->Tiles) {
    Result = TileChunk->Tiles[P.RelTileY * TileMap->TileChunkDim + P.RelTileX];
  }

  return Result;
}

internal inline tile_map_position
RecanonicalizePosition(tile_map *TileMap, tile_map_position Pos) {
  tile_map_position Result = Pos;
  RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.Offset_.X);
  RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.Offset_.Y);
  return Result;
}

bool32
IsTileValueEmpty(uint32 Value) {
  bool32 Empty = (Value == 1) || (Value == 3);
  return Empty;
}

// 0: nothing
// 1: empty
// 2: block
// 3: stair
bool32
IsTileMapEmtpy(tile_map *TileMap, tile_map_position Pos) {
  bool32 Empty = false;
  uint32 TileValue = GetTileValue(TileMap, Pos.AbsTileX, Pos.AbsTileY, Pos.AbsTileZ);
  Empty = IsTileValueEmpty(TileValue);
  return Empty;
}

inline void
SetTileValue(memory_arena *Arena, tile_map *TileMap,
  uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ,
  uint32 TileValue
) {
  tile_chunk_position P = GetTileChunkPosition(TileMap, AbsTileX, AbsTileY, AbsTileZ);
  tile_chunk *TileChunk = GetTileChunk(TileMap, P.TileChunkX, P.TileChunkY, P.AbsTileZ, Arena);
  Assert(TileChunk);
  TileChunk->Tiles[P.RelTileY * TileMap->TileChunkDim + P.RelTileX] = TileValue;
}

inline bool32
AreSameTiles(tile_map_position P1, tile_map_position P2) {
  return P1.AbsTileX == P2.AbsTileX &&
    P1.AbsTileY == P2.AbsTileY &&
    P1.AbsTileZ == P2.AbsTileZ;
}

inline tile_map_diff
SubtractPosition(tile_map *TileMap, tile_map_position P1, tile_map_position P2) {
  tile_map_diff Result = {};
  Result.dXY.X = TileMap->TileSizeInMeters * ((real32)P1.AbsTileX - (real32)P2.AbsTileX) + P1.Offset_.X - P2.Offset_.X;
  Result.dXY.Y = TileMap->TileSizeInMeters * ((real32)P1.AbsTileY - (real32)P2.AbsTileY) + P1.Offset_.Y - P2.Offset_.Y;
  return Result;
}

inline tile_map_position
CenteredTilePoint(uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  tile_map_position Result = {};
  Result.AbsTileX = AbsTileX;
  Result.AbsTileY = AbsTileY;
  Result.AbsTileZ = AbsTileZ;
  return Result;
}

inline tile_map_position
Offset(tile_map *TileMap, tile_map_position Pos, v2 Offset) {
  Pos.Offset_ += Offset;
  Pos = RecanonicalizePosition(TileMap, Pos);
  return Pos;
}
