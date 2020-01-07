#include "handmade_world.h"

inline world_chunk *
GetWorldChunk(world *World, int32 ChunkX, int32 ChunkY, int32 AbsTileZ, memory_arena *Arena=NULL) {
  uint32 HashIndex = (ChunkX*19 + ChunkY*7 + AbsTileZ*3) & (ArrayCount(World->ChunkHash) - 1);
  world_chunk *Chunk = World->ChunkHash[HashIndex];

  while(Chunk) {
    if(Chunk->ChunkX == ChunkX &&
      Chunk->ChunkY == ChunkY &&
      Chunk->AbsTileZ == AbsTileZ
    ) {
      break;
    }

    Chunk = Chunk->NextInHash;
  }

  if(Chunk == NULL && Arena) {
    world_chunk *NewChunk = PushStruct(Arena, world_chunk);
    NewChunk->ChunkX = ChunkX;
    NewChunk->ChunkY = ChunkY;
    NewChunk->AbsTileZ = AbsTileZ;
    NewChunk->NextInHash = World->ChunkHash[HashIndex];
    World->ChunkHash[HashIndex] = NewChunk;

    return NewChunk;
  }

  return Chunk;
}

internal inline void
RecanonicalizeCoord(world *World, int32 *Tile, real32 *TileRel) {
  // NOTE: tile map is assumed to be toroidal
  int32 Offset = RoundReal32ToInt32(*TileRel / World->TileSizeInMeters);
  *Tile += Offset;
  *TileRel -= Offset * World->TileSizeInMeters;

  Assert(*TileRel >= -0.50001f*World->TileSizeInMeters);
  Assert(*TileRel < 0.50001f*World->TileSizeInMeters);
}

internal inline world_position
RecanonicalizePosition(world *World, world_position Pos) {
  world_position Result = Pos;
  RecanonicalizeCoord(World, &Result.AbsTileX, &Result.Offset_.X);
  RecanonicalizeCoord(World, &Result.AbsTileY, &Result.Offset_.Y);
  return Result;
}

inline world_diff
SubtractPosition(world *World, world_position P1, world_position P2) {
  world_diff Result = {};
  Result.dXY.X = World->TileSizeInMeters * ((real32)P1.AbsTileX - (real32)P2.AbsTileX) + P1.Offset_.X - P2.Offset_.X;
  Result.dXY.Y = World->TileSizeInMeters * ((real32)P1.AbsTileY - (real32)P2.AbsTileY) + P1.Offset_.Y - P2.Offset_.Y;
  return Result;
}

inline world_position
Offset(world *World, world_position Pos, v2 Offset) {
  Pos.Offset_ += Offset;
  Pos = RecanonicalizePosition(World, Pos);
  return Pos;
}
