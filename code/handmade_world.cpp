#include "handmade_world.h"

inline world_position
NullPosition() {
  world_position result = {};
  result.chunkX = INT32_MAX;
  return result;
}

inline bool32
IsPositionValid(world_position p) {
  return p.chunkX != INT32_MAX;
}

inline world_chunk *
GetWorldChunk(
  game_world *world,
  int32 chunkX,
  int32 chunkY,
  int32 chunkZ,
  memory_arena *arena=NULL
) {
  uint32 hashIndex = (chunkX*19 + chunkY*7 + chunkZ*3) & (ArrayCount(world->chunkHash) - 1);
  world_chunk *chunk = world->chunkHash[hashIndex];

  while(chunk) {
    if(chunk->chunkX == chunkX &&
      chunk->chunkY == chunkY &&
      chunk->chunkZ == chunkZ
    ) {
      break;
    }

    chunk = chunk->next;
  }

  if(chunk == NULL && arena) {
    world_chunk *newChunk = PushStruct(arena, world_chunk);
    newChunk->chunkX = chunkX;
    newChunk->chunkY = chunkY;
    newChunk->chunkZ = chunkZ;
    newChunk->next = world->chunkHash[hashIndex];
    world->chunkHash[hashIndex] = newChunk;

    return newChunk;
  }

  return chunk;
}

inline bool32
IsCanonicalCoord(game_world *world, real32 value) {
  bool32 result = (value >= 0) &&
    (value <= world->chunkSizeInMeters);
  return result;
}

inline bool32
IsCanonicalPosition(game_world *world, world_position p) {
  bool32 result = IsCanonicalCoord(world, p.offset_.x) && IsCanonicalCoord(world, p.offset_.y);
  return result;
}

internal inline void
RecanonicalizeCoord(
  game_world *world,
  int32 *chunk,
  real32 *chunkRel
) {
  // NOTE: game_world is not allowd to be wrapped
  int32 offset = FloorReal32ToInt32(*chunkRel / world->chunkSizeInMeters);
  *chunk += offset;
  *chunkRel -= offset * world->chunkSizeInMeters;

  Assert(*chunkRel >= 0);
  Assert(*chunkRel <= world->chunkSizeInMeters);
}

internal inline world_position
RecanonicalizePosition(game_world *world, world_position pos) {
  world_position result = pos;
  RecanonicalizeCoord(world, &result.chunkX, &result.offset_.x);
  RecanonicalizeCoord(world, &result.chunkY, &result.offset_.y);
  return result;
}

inline world_diff
SubtractPosition(game_world *world, world_position p1, world_position p2) {
  world_diff result = {};
  result.dXY.x = world->chunkSizeInMeters * ((real32)p1.chunkX - (real32)p2.chunkX) + p1.offset_.x - p2.offset_.x;
  result.dXY.y = world->chunkSizeInMeters * ((real32)p1.chunkY - (real32)p2.chunkY) + p1.offset_.y - p2.offset_.y;
  return result;
}

inline world_position
MapIntoWorldSpace(game_world *world, world_position pos, v2 offset) {
  pos.offset_ += offset;
  pos = RecanonicalizePosition(world, pos);
  return pos;
}

inline world_position
WorldPositionFromTilePosition(
  game_world *world,
  int32 tileX,
  int32 tileY,
  int32 tileZ
) {
  world_position result = {};
  result.chunkX = FloorReal32ToInt32((real32)tileX / (real32)TILES_PER_CHUNK);
  result.chunkY = FloorReal32ToInt32((real32)tileY / (real32)TILES_PER_CHUNK);
  result.chunkZ = tileZ;

  result.offset_.x = world->tileSizeInMeters * (tileX - result.chunkX*TILES_PER_CHUNK);
  result.offset_.y = world->tileSizeInMeters * (tileY - result.chunkY*TILES_PER_CHUNK);

  Assert(IsCanonicalPosition(world, result));

  return result;
}

// lowEntity may be NULL
internal void
ChangeEntityLocation(
  memory_arena *arena, game_world *world,
  low_entity *lowEntity, world_position *oldP,
  world_position *newP
){
  Assert(lowEntity);
  if(oldP) {
    Assert(IsCanonicalPosition(world, *oldP));
  }
  Assert(IsCanonicalPosition(world, *newP));

  if(oldP) {
    world_chunk *chunk = GetWorldChunk(world, oldP->chunkX, oldP->chunkY, oldP->chunkZ);
    Assert(chunk);
    entity_block *firstBlock = &chunk->entityBlock;
    entity_block *block = &chunk->entityBlock;
    for(; block; block = block->next) {
      for(uint32 index=0; index < block->entityCount; index++) {
        if(block->lowEntities[index] == lowEntity) {
          Assert(firstBlock->entityCount > 0);
          block->lowEntities[index] = firstBlock->lowEntities[--firstBlock->entityCount];

          if(firstBlock->entityCount == 0) {
            // TODO: figure out why?
            Assert(firstBlock->next);
            entity_block *nextBlock = firstBlock->next;
            *firstBlock = *nextBlock;

            nextBlock->next = world->firstFree;
            world->firstFree = nextBlock;
          }

          block = NULL;
          break;
        }
      }
    }
  }

  world_chunk *chunk = GetWorldChunk(world, newP->chunkX, newP->chunkY, newP->chunkZ, arena);
  Assert(chunk);
  entity_block *firstBlock = &chunk->entityBlock;
  if(firstBlock->entityCount == ArrayCount(firstBlock->lowEntities)) {
    entity_block *newBlock = world->firstFree;
    if(newBlock) {
      world->firstFree = newBlock->next;
    } else {
      newBlock = PushStruct(arena, entity_block);
    }
    *newBlock = *firstBlock;
    firstBlock->next = newBlock;
    firstBlock->entityCount = 0;
  }

  Assert(firstBlock->entityCount < ArrayCount(firstBlock->lowEntities));
  firstBlock->lowEntities[firstBlock->entityCount++] = lowEntity;
}
