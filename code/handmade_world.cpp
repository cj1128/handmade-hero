#include "handmade_world.h"

inline world_position
NullPosition()
{
  world_position result = {};
  result.chunkX = INT32_MAX;
  return result;
}

inline bool32
IsValid(world_position *p)
{
  return p->chunkX != INT32_MAX;
}

inline bool32
IsValid(v3 p)
{
  return p.x != INVALID_P.x && p.y != INVALID_P.y && p.z != INVALID_P.z;
}

inline world_position
CenteredChunkPoint(game_world *world, int32 chunkX, int32 chunkY, int32 chunkZ)
{
  world_position result = {};

  result.chunkX = chunkX;
  result.chunkY = chunkY;
  result.chunkZ = chunkZ;

  return result;
}

inline world_position
CenteredChunkPoint(game_world *world, world_chunk *chunk)
{
  CenteredChunkPoint(world, chunk->chunkX, chunk->chunkY, chunk->chunkZ);
}

inline world_chunk *
GetWorldChunk(game_world *world,
  int32 chunkX,
  int32 chunkY,
  int32 chunkZ,
  memory_arena *arena = NULL)
{
  uint32 hashIndex = (chunkX * 19 + chunkY * 7 + chunkZ * 3)
    & (ArrayCount(world->chunkHash) - 1);
  world_chunk *chunk = world->chunkHash[hashIndex];

  while(chunk) {
    if(chunk->chunkX == chunkX && chunk->chunkY == chunkY
      && chunk->chunkZ == chunkZ) {
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
IsCanonicalCoord(real32 chunkDim, real32 value)
{
  real32 epsilon = 0.01f;
  real32 r = 0.5f * chunkDim + epsilon;
  bool32 result = (value >= -r) && (value <= r);
  return result;
}

inline bool32
IsCanonicalPosition(game_world *world, world_position *p)
{
  Assert(IsValid(p));
  bool32 result = IsCanonicalCoord(world->chunkDimInMeters.x, p->offset.x)
    && IsCanonicalCoord(world->chunkDimInMeters.y, p->offset.y)
    && IsCanonicalCoord(world->chunkDimInMeters.z, p->offset.z);
  return result;
}

inline bool32
AreInSameChunk(game_world *world, world_position *p1, world_position *p2)
{
  // Assert(IsCanonicalCoord(world, p1));
  // Assert(IsCanonicalPosition(world, p2));

  bool32 result = ((p1->chunkX == p2->chunkX) && (p1->chunkY == p2->chunkY)
    && (p1->chunkZ == p2->chunkZ));

  return result;
}

internal inline void
RecanonicalizeCoord(real32 chunkDim, int32 *chunk, real32 *chunkRel)
{
  // NOTE(cj): game_world is not allowd to be wrapped

  int32 offset = RoundReal32ToInt32(*chunkRel / chunkDim);
  *chunk += offset;
  *chunkRel -= offset * chunkDim;

  Assert(IsCanonicalCoord(chunkDim, *chunkRel));
}

internal inline world_position
RecanonicalizePosition(game_world *world, world_position pos)
{
  world_position result = pos;
  RecanonicalizeCoord(world->chunkDimInMeters.x,
    &result.chunkX,
    &result.offset.x);
  RecanonicalizeCoord(world->chunkDimInMeters.y,
    &result.chunkY,
    &result.offset.y);
  RecanonicalizeCoord(world->chunkDimInMeters.z,
    &result.chunkZ,
    &result.offset.z);
  return result;
}

inline v3
SubtractPosition(game_world *world, world_position p1, world_position p2)
{
  v3 dChunk = {
    (real32)p1.chunkX - (real32)p2.chunkX,
    (real32)p1.chunkY - (real32)p2.chunkY,
    (real32)p1.chunkZ - (real32)p2.chunkZ,
  };
  v3 result
    = Hadamard(world->chunkDimInMeters, dChunk) + (p1.offset - p2.offset);
  return result;
}

inline world_position
MapIntoWorldSpace(game_world *world, world_position pos, v3 offset)
{
  pos.offset += offset;
  pos = RecanonicalizePosition(world, pos);
  return pos;
}

// oldP, newP may be null
internal void
ChangeEntityLocationRaw(memory_arena *arena,
  game_world *world,
  stored_entity *stored,
  world_position *oldP,
  world_position *newP)
{
  Assert(stored);
  Assert(!oldP || IsCanonicalPosition(world, oldP));
  Assert(!newP || IsCanonicalPosition(world, newP));

  if(oldP && newP && AreInSameChunk(world, oldP, newP)) {
    return;
  }

  if(oldP) {
    world_chunk *chunk
      = GetWorldChunk(world, oldP->chunkX, oldP->chunkY, oldP->chunkZ);
    Assert(chunk);
    entity_block *firstBlock = &chunk->entityBlock;
    entity_block *block = &chunk->entityBlock;
    bool32 notFound = true;
    for(; block && notFound; block = block->next) {
      for(uint32 index = 0; index < block->entityCount; index++) {
        if(block->entities[index] == stored) {
          Assert(firstBlock->entityCount > 0);
          block->entities[index]
            = firstBlock->entities[--firstBlock->entityCount];

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
    Assert(!notFound);
  }

  if(newP) {
    world_chunk *chunk
      = GetWorldChunk(world, newP->chunkX, newP->chunkY, newP->chunkZ, arena);
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
  world_position newPInit)
{
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
    ClearFlags(&stored->sim, EntityFlag_NonSpatial);
  } else {
    stored->p = NullPosition();
    AddFlags(&stored->sim, EntityFlag_NonSpatial);
  }
}
