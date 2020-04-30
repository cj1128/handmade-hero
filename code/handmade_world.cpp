#include "handmade_world.h"

inline world_position
NullPosition() {
  world_position result = {};
  result.chunkX = INT32_MAX;
  return result;
}

inline bool32
IsValid(world_position *p) {
  return p->chunkX != INT32_MAX;
}

inline bool32
IsValid(v3 p) {
  return p.x != INVALID_P.x && p.y != INVALID_P.y && p.z != INVALID_P.z;
}

inline world_chunk *
GetWorldChunk(game_world *world,
              int32 chunkX,
              int32 chunkY,
              int32 chunkZ,
              memory_arena *arena = NULL) {
  uint32 hashIndex = (chunkX * 19 + chunkY * 7 + chunkZ * 3) &
                     (ArrayCount(world->chunkHash) - 1);
  world_chunk *chunk = world->chunkHash[hashIndex];

  while(chunk) {
    if(chunk->chunkX == chunkX && chunk->chunkY == chunkY &&
       chunk->chunkZ == chunkZ) {
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
IsCanonicalCoord(real32 chunkDim, real32 value) {
  bool32 result = (value >= 0) && (value <= chunkDim);
  return result;
}

inline bool32
IsCanonicalPosition(game_world *world, world_position *p) {
  Assert(IsValid(p));
  bool32 result = IsCanonicalCoord(world->chunkDimInMeters.x, p->_offset.x) &&
                  IsCanonicalCoord(world->chunkDimInMeters.y, p->_offset.y) &&
                  IsCanonicalCoord(world->chunkDimInMeters.z, p->_offset.z);
  return result;
}

inline bool32
AreInSameChunk(game_world *world, world_position *p1, world_position *p2) {
  Assert(IsCanonicalPosition(world, p1));
  Assert(IsCanonicalPosition(world, p2));

  bool32 result = ((p1->chunkX == p2->chunkX) && (p1->chunkY == p2->chunkY) &&
                   (p1->chunkZ == p2->chunkZ));

  return result;
}

internal inline void
RecanonicalizeCoord(real32 chunkDim, int32 *chunk, real32 *chunkRel) {
  // NOTE: game_world is not allowd to be wrapped
  int32 offset = FloorReal32ToInt32(*chunkRel / chunkDim);
  *chunk += offset;
  *chunkRel -= offset * chunkDim;

  Assert(*chunkRel >= 0);
  Assert(*chunkRel <= chunkDim);
}

internal inline world_position
RecanonicalizePosition(game_world *world, world_position pos) {
  world_position result = pos;
  RecanonicalizeCoord(world->chunkDimInMeters.x,
                      &result.chunkX,
                      &result._offset.x);
  RecanonicalizeCoord(world->chunkDimInMeters.y,
                      &result.chunkY,
                      &result._offset.y);
  RecanonicalizeCoord(world->chunkDimInMeters.z,
                      &result.chunkZ,
                      &result._offset.z);
  return result;
}

inline v3
SubtractPosition(game_world *world, world_position p1, world_position p2) {
  v3 dChunk = {
    (real32)p1.chunkX - (real32)p2.chunkX,
    (real32)p1.chunkY - (real32)p2.chunkY,
    (real32)p1.chunkZ - (real32)p2.chunkZ,
  };
  v3 result =
    Hadamard(world->chunkDimInMeters, dChunk) + (p1._offset - p2._offset);
  return result;
}

inline world_position
MapIntoWorldSpace(game_world *world, world_position pos, v3 offset) {
  pos._offset += offset;
  pos = RecanonicalizePosition(world, pos);
  return pos;
}

inline world_position
WorldPositionFromTilePosition(game_world *world,
                              int32 tileX,
                              int32 tileY,
                              int32 tileZ) {
  world_position result = {};
  result.chunkX = FloorReal32ToInt32((real32)tileX / (real32)TILES_PER_CHUNK);
  result.chunkY = FloorReal32ToInt32((real32)tileY / (real32)TILES_PER_CHUNK);
  result.chunkZ = tileZ;

  result._offset.x =
    world->tileSizeInMeters * (tileX - result.chunkX * TILES_PER_CHUNK);
  result._offset.y =
    world->tileSizeInMeters * (tileY - result.chunkY * TILES_PER_CHUNK);

  Assert(IsCanonicalPosition(world, &result));

  return result;
}

// oldP, newP may be null
internal void
ChangeEntityLocationRaw(memory_arena *arena,
                        game_world *world,
                        stored_entity *stored,
                        world_position *oldP,
                        world_position *newP) {
  Assert(stored);
  Assert(!oldP || IsCanonicalPosition(world, oldP));
  Assert(!newP || IsCanonicalPosition(world, newP));

  if(oldP && newP && AreInSameChunk(world, oldP, newP)) {
    return;
  }

  if(oldP) {
    world_chunk *chunk =
      GetWorldChunk(world, oldP->chunkX, oldP->chunkY, oldP->chunkZ);
    Assert(chunk);
    entity_block *firstBlock = &chunk->entityBlock;
    entity_block *block = &chunk->entityBlock;
    bool32 notFound = true;
    for(; block && notFound; block = block->next) {
      for(uint32 index = 0; index < block->entityCount; index++) {
        if(block->entities[index] == stored) {
          Assert(firstBlock->entityCount > 0);
          block->entities[index] =
            firstBlock->entities[--firstBlock->entityCount];

          if(firstBlock->entityCount == 0 && firstBlock->next) {
            entity_block *nextBlock = firstBlock->next;
            *firstBlock = *nextBlock;

            nextBlock->next = world->firstFree;
            world->firstFree = nextBlock;
          }

          notFound = false;
        }
      }
    }
  }

  if(newP) {
    world_chunk *chunk =
      GetWorldChunk(world, newP->chunkX, newP->chunkY, newP->chunkZ, arena);
    Assert(chunk);
    entity_block *firstBlock = &chunk->entityBlock;
    if(firstBlock->entityCount == ArrayCount(firstBlock->entities)) {
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

    Assert(firstBlock->entityCount < ArrayCount(firstBlock->entities));
    firstBlock->entities[firstBlock->entityCount++] = stored;
  }
}

internal void
ChangeEntityLocation(memory_arena *arena,
                     game_world *world,
                     stored_entity *stored,
                     world_position newPInit) {
  world_position *oldP = NULL;
  world_position *newP = NULL;

  if(IsValid(&stored->p)) {
    oldP = &stored->p;
  }

  if(IsValid(&newPInit)) {
    newP = &newPInit;
  }

  ChangeEntityLocationRaw(arena, world, stored, oldP, newP);

  if(newP) {
    stored->p = *newP;
    ClearFlag(&stored->sim, EntityFlag_NonSpatial);
  } else {
    stored->p = NullPosition();
    AddFlag(&stored->sim, EntityFlag_NonSpatial);
  }
}
