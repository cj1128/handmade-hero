#include "handmade_world.h"

inline world_chunk *
GetWorldChunk(world *World, int32 ChunkX, int32 ChunkY, int32 ChunkZ, memory_arena *Arena=NULL) {
  uint32 HashIndex = (ChunkX*19 + ChunkY*7 + ChunkZ*3) & (ArrayCount(World->ChunkHash) - 1);
  world_chunk *Chunk = World->ChunkHash[HashIndex];

  while(Chunk) {
    if(Chunk->ChunkX == ChunkX &&
      Chunk->ChunkY == ChunkY &&
      Chunk->ChunkZ == ChunkZ
    ) {
      break;
    }

    Chunk = Chunk->Next;
  }

  if(Chunk == NULL && Arena) {
    world_chunk *NewChunk = PushStruct(Arena, world_chunk);
    NewChunk->ChunkX = ChunkX;
    NewChunk->ChunkY = ChunkY;
    NewChunk->ChunkZ = ChunkZ;
    NewChunk->Next = World->ChunkHash[HashIndex];
    World->ChunkHash[HashIndex] = NewChunk;

    return NewChunk;
  }

  return Chunk;
}

inline bool32
IsCanonicalCoord(world *World, real32 Value) {
  bool32 Result = (Value >= 0) &&
    (Value <= World->ChunkSizeInMeters);
  return Result;
}

inline bool32
IsCanonicalPosition(world *World, world_position P) {
  bool32 Result = IsCanonicalCoord(World, P.Offset_.X) && IsCanonicalCoord(World, P.Offset_.Y);
  return Result;
}

internal inline void
RecanonicalizeCoord(world *World, int32 *Chunk, real32 *ChunkRel) {
  // NOTE: world is not allowd to be wrapped
  int32 Offset = FloorReal32ToInt32(*ChunkRel / World->ChunkSizeInMeters);
  *Chunk += Offset;
  *ChunkRel -= Offset * World->ChunkSizeInMeters;

  Assert(*ChunkRel >= 0);
  Assert(*ChunkRel <= World->ChunkSizeInMeters);
}

internal inline world_position
RecanonicalizePosition(world *World, world_position Pos) {
  world_position Result = Pos;
  RecanonicalizeCoord(World, &Result.ChunkX, &Result.Offset_.X);
  RecanonicalizeCoord(World, &Result.ChunkY, &Result.Offset_.Y);
  return Result;
}

inline world_diff
SubtractPosition(world *World, world_position P1, world_position P2) {
  world_diff Result = {};
  Result.dXY.X = World->ChunkSizeInMeters * ((real32)P1.ChunkX - (real32)P2.ChunkX) + P1.Offset_.X - P2.Offset_.X;
  Result.dXY.Y = World->ChunkSizeInMeters * ((real32)P1.ChunkY - (real32)P2.ChunkY) + P1.Offset_.Y - P2.Offset_.Y;
  return Result;
}

inline world_position
MapIntoWorldSpace(world *World, world_position Pos, v2 Offset) {
  Pos.Offset_ += Offset;
  Pos = RecanonicalizePosition(World, Pos);
  return Pos;
}

inline world_position
WorldPositionFromTilePosition(world *World, int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ) {
  world_position Result = {};
  Result.ChunkX = FloorReal32ToInt32((real32)AbsTileX / (real32)TILES_PER_CHUNK);
  Result.ChunkY = FloorReal32ToInt32((real32)AbsTileY / (real32)TILES_PER_CHUNK);
  Result.ChunkZ = AbsTileZ;

  Result.Offset_.X = World->TileSizeInMeters * (AbsTileX - Result.ChunkX*TILES_PER_CHUNK);
  Result.Offset_.Y = World->TileSizeInMeters * (AbsTileY - Result.ChunkY*TILES_PER_CHUNK);

  return Result;
}

// LowEntity may be NULL
internal void
ChangeEntityLocation(
  memory_arena *Arena, world *World,
  low_entity *LowEntity, world_position *OldP,
  world_position *NewP
){
  Assert(LowEntity);
  if(OldP) {
    Assert(IsCanonicalPosition(World, *OldP));
  }
  Assert(IsCanonicalPosition(World, *NewP));

  if(OldP) {
    world_chunk *Chunk = GetWorldChunk(World, OldP->ChunkX, OldP->ChunkY, OldP->ChunkZ);
    Assert(Chunk);
    entity_block *FirstBlock = &Chunk->EntityBlock;
    entity_block *Block = &Chunk->EntityBlock;
    for(; Block; Block = Block->Next) {
      for(uint32 Index=0; Index < Block->EntityCount; Index++) {
        if(Block->LowEntities[Index] == LowEntity) {
          Assert(FirstBlock);
          Block->LowEntities[Index] = FirstBlock->LowEntities[--FirstBlock->EntityCount];

          if(FirstBlock->EntityCount == 0) {
            Assert(FirstBlock->Next);
            entity_block *NextBlock = FirstBlock->Next;
            *FirstBlock = *NextBlock;

            NextBlock->Next = World->FirstFree;
            World->FirstFree = NextBlock;
          }

          Block = NULL;
          break;
        }
      }
    }
  }

  world_chunk *Chunk = GetWorldChunk(World, NewP->ChunkX, NewP->ChunkY, NewP->ChunkZ, Arena);
  Assert(Chunk);
  entity_block *FirstBlock = &Chunk->EntityBlock;
  if(FirstBlock->EntityCount == ArrayCount(FirstBlock->LowEntities)) {
    entity_block *NewBlock = World->FirstFree;
    if(NewBlock) {
      World->FirstFree = NewBlock->Next;
    } else {
      NewBlock = PushStruct(Arena, entity_block);
    }
    *NewBlock = *FirstBlock;
    FirstBlock->Next = NewBlock;
    FirstBlock->EntityCount = 0;
  }

  Assert(FirstBlock->EntityCount < ArrayCount(FirstBlock->LowEntities));
  FirstBlock->LowEntities[FirstBlock->EntityCount++] = LowEntity;
}
