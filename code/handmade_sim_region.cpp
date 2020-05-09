internal bool32
TestWall(real32 wallX,
  real32 deltaX,
  real32 deltaY,
  real32 relX,
  real32 relY,
  real32 radiusY,
  real32 *tMin)
{
  real32 tEpsilon = 0.001f;
  bool32 hit = false;

  if(deltaX != 0.0f) {
    real32 tResult = (wallX - relX) / deltaX;
    real32 y = relY + tResult * deltaY;
    if(y >= -radiusY && y <= radiusY) {
      if(tResult > 0.0f && tResult < *tMin) {
        hit = true;
        *tMin = Maximum(0.0f, tResult - tEpsilon);
      }
    }
  }

  return hit;
}

internal v3
GetSimSpaceP(sim_region *simRegion, stored_entity *entity)
{
  v3 result = INVALID_P;

  if(IsValid(&entity->p)) {
    result = SubtractPosition(simRegion->world, entity->p, simRegion->origin);
  }

  return result;
}

internal sim_region_hash *
GetHashFromStored(sim_region *simRegion, stored_entity *stored)
{
  Assert(stored);

  sim_region_hash *result = NULL;

  uint32 hashValue = (uint32)((uint8 *)stored - (uint8 *)simRegion);

  for(int offset = 0; offset < ArrayCount(simRegion->hash); offset++) {
    uint32 hashMask = ArrayCount(simRegion->hash) - 1;
    uint32 hashIndex = (hashValue + offset) & hashMask;
    sim_region_hash *entry = simRegion->hash + hashIndex;

    if(entry->stored == NULL || entry->stored == stored) {
      result = entry;
      break;
    }
  }

  return result;
}

internal bool32
EntityOverlapsRectangle(v3 p, v3 dim, rectangle3 rect)
{
  rectangle3 grown = AddRadius(rect, 0.5f * dim);
  bool32 result = IsInRectangle(grown, p);
  return result;
}

internal void LoadEntityReference(sim_region *simRegion, entity_reference *ref);
internal sim_entity *
AddEntityToSimRegion(sim_region *simRegion, stored_entity *stored, v3 p)
{
  Assert(stored);
  Assert(stored->sim.dim.x <= 2 * simRegion->maxEntityRadius);
  Assert(stored->sim.dim.y <= 2 * simRegion->maxEntityRadius);
  Assert(stored->sim.dim.z <= 2 * simRegion->maxEntityRadius);

  sim_entity *result = NULL;

  sim_region_hash *entry = GetHashFromStored(simRegion, stored);
  if(entry->stored == NULL) {
    Assert(simRegion->entityCount < ArrayCount(simRegion->entities));
    sim_entity *entity = simRegion->entities + simRegion->entityCount++;
    *entity = stored->sim;
    entity->stored = stored;

    LoadEntityReference(simRegion, &entity->sword);

    Assert(!HasFlag(&stored->sim, EntityFlag_Simming));
    AddFlag(&stored->sim, EntityFlag_Simming);

    if(IsValid(p)) {
      entity->updatable
        = EntityOverlapsRectangle(p, entity->dim, simRegion->updatableBounds);
    }

    entry->entity = entity;
    entry->stored = stored;

    result = entity;
  } else {
    result = entry->entity;
  }

  Assert(result);
  result->p = p;

  return result;
}

inline void
StoreEntityReference(entity_reference *ref)
{
  if(ref->entity) {
    ref->stored = ref->entity->stored;
  }
}

internal void
LoadEntityReference(sim_region *simRegion, entity_reference *ref)
{
  if(ref->stored) {
    ref->entity = AddEntityToSimRegion(simRegion,
      ref->stored,
      GetSimSpaceP(simRegion, ref->stored));
  }
}

internal sim_region *
BeginSim(memory_arena *arena,
  game_state *state,
  world_position origin,
  rectangle3 bounds,
  real32 dt)
{
  sim_region *simRegion = PushStruct(arena, sim_region);
  simRegion->entityCount = 0;
  ZeroStruct(simRegion->hash);

  simRegion->maxEntityRadius = 5.0f;
  simRegion->maxEntityVelocity = 30.0f;

  simRegion->world = &state->world;
  simRegion->origin = origin;
  simRegion->updatableBounds = AddRadius(bounds,
    v3{ simRegion->maxEntityRadius,
      simRegion->maxEntityRadius,
      simRegion->maxEntityRadius });

  real32 updateSafetyMargin
    = simRegion->maxEntityRadius + dt * simRegion->maxEntityVelocity;
  real32 updateSafetyMarginZ = 1.0f;
  simRegion->bounds = AddRadius(simRegion->updatableBounds,
    v3{ updateSafetyMargin, updateSafetyMargin, updateSafetyMarginZ });

  world_position minChunk
    = MapIntoWorldSpace(simRegion->world, origin, bounds.min);
  world_position maxChunk
    = MapIntoWorldSpace(simRegion->world, origin, bounds.max);

  for(int32 chunkY = minChunk.chunkY; chunkY <= maxChunk.chunkY; chunkY++) {
    for(int32 chunkX = minChunk.chunkX; chunkX <= maxChunk.chunkX; chunkX++) {
      world_chunk *chunk
        = GetWorldChunk(simRegion->world, chunkX, chunkY, origin.chunkZ);

      if(chunk) {
        for(entity_block *block = &chunk->entityBlock; block;
            block = block->next) {
          for(uint32 index = 0; index < block->entityCount; index++) {
            stored_entity *stored = block->entities[index];
            Assert(stored);

            if(!HasFlag(&stored->sim, EntityFlag_NonSpatial)) {
              v3 simSpaceP = GetSimSpaceP(simRegion, stored);

              if(EntityOverlapsRectangle(simSpaceP,
                   stored->sim.dim,
                   simRegion->bounds)) {
                AddEntityToSimRegion(simRegion, stored, simSpaceP);
              }
            }
          }
        }
      }
    }
  }

  return simRegion;
}

internal void
ClearCollisionRulesFor(game_state *state, stored_entity *entity)
{
  // TODO: need a better way than linear search
  for(uint32 bucket = 0; bucket < ArrayCount(state->collisionRuleHash);
      bucket++) {
    for(pairwise_collision_rule **rule = &state->collisionRuleHash[bucket];
        *rule;) {
      if((*rule)->a == entity || (*rule)->b == entity) {
        pairwise_collision_rule *removed = *rule;
        *rule = (*rule)->nextInHash;
        removed->nextInHash = state->firstFreeCollisionRule;
        state->firstFreeCollisionRule = removed;
      } else {
        rule = &(*rule)->nextInHash;
      }
    }
  }
}

internal void
AddCollisionRule(game_state *state,
  stored_entity *a,
  stored_entity *b,
  bool32 shouldCollide)
{
  if(a > b) {
    stored_entity *tmp = a;
    a = b;
    b = tmp;
  }

  pairwise_collision_rule *found = NULL;
  uint32 bucket = (uintptr_t)a & (ArrayCount(state->collisionRuleHash) - 1);
  for(pairwise_collision_rule *rule = state->collisionRuleHash[bucket]; rule;
      rule = rule->nextInHash) {
    if(rule->a == a && rule->b == b) {
      found = rule;
      break;
    }
  }

  if(!found) {
    found = state->firstFreeCollisionRule;

    if(found) {
      state->firstFreeCollisionRule = found->nextInHash;
    } else {
      found = PushStruct(&state->worldArena, pairwise_collision_rule);
    }

    found->nextInHash = state->collisionRuleHash[bucket];
    state->collisionRuleHash[bucket] = found;
  }

  found->a = a;
  found->b = b;
  found->shouldCollide = shouldCollide;
}

internal bool32
ShouldCollide(game_state *state, stored_entity *a, stored_entity *b)
{
  bool32 result = true;

  if(a->sim.type > b->sim.type) {
    stored_entity *tmp = a;
    a = b;
    b = tmp;
  }

  uint32 hashBucket = (uintptr_t)a & (ArrayCount(state->collisionRuleHash) - 1);
  for(pairwise_collision_rule *rule = state->collisionRuleHash[hashBucket];
      rule;
      rule = rule->nextInHash) {
    if(rule->a == a && rule->b == b) {
      result = rule->shouldCollide;
    }
  }

  return result;
}

internal void
MoveEntity(game_state *state,
  sim_region *simRegion,
  move_spec *spec,
  sim_entity *entity,
  real32 dt,
  v3 ddP)
{
  Assert(entity);

  game_world *world = simRegion->world;

  // Fix diagonal movement problem
  if(spec->unitddP) {
    real32 ddPLength = Length(ddP);
    if(ddPLength > 1.0f) {
      ddP *= (1.0f / ddPLength);
    }
  }

  Assert(LengthSq(entity->dP) <= Square(simRegion->maxEntityVelocity));

  ddP *= spec->ddPScale;
  ddP += -spec->drag * entity->dP;
  ddP += v3{ 0, 0, -9.8f }; // Gravity

  v3 entityDelta = 0.5f * ddP * Square(dt) + entity->dP * dt;
  entity->dP += ddP * dt;

  real32 distanceRemaining = entity->distanceLimit;

  if(distanceRemaining == 0.0f) {
    distanceRemaining = 1000.0f;
  }

  for(int32 Iteration = 0; Iteration < 4; Iteration++) {
    real32 tMin = 1.0f;

    real32 entityDeltaLength = Length(entityDelta);

    if(entityDeltaLength == 0.0f) {
      break;
    }

    if(entityDeltaLength > distanceRemaining) {
      tMin = distanceRemaining / entityDeltaLength;
    }

    sim_entity *hitEntity = NULL;
    v3 WallNormal = {};
    v3 targetEntityP = entity->p + entityDelta;

    for(uint32 index = 0; index < simRegion->entityCount; index++) {
      sim_entity *testEntity = simRegion->entities + index;
      if(testEntity != entity) {
        if(!HasFlag(testEntity, EntityFlag_NonSpatial)
          && ShouldCollide(state, entity->stored, testEntity->stored)) {
          v3 Rel = entity->p - testEntity->p;

          real32 radiusW = 0.5f * testEntity->dim.x + 0.5f * entity->dim.x;
          real32 radiusH = 0.5f * testEntity->dim.y + 0.5f * entity->dim.y;

          // left
          if(TestWall(-radiusW,
               entityDelta.x,
               entityDelta.y,
               Rel.x,
               Rel.y,
               radiusH,
               &tMin)) {
            WallNormal = { 1, 0, 0 };
            hitEntity = testEntity;
          }

          // right
          if(TestWall(radiusW,
               entityDelta.x,
               entityDelta.y,
               Rel.x,
               Rel.y,
               radiusH,
               &tMin)) {
            WallNormal = { -1, 0, 0 };
            hitEntity = testEntity;
          }

          // bottom
          if(TestWall(-radiusH,
               entityDelta.y,
               entityDelta.x,
               Rel.y,
               Rel.x,
               radiusW,
               &tMin)) {
            WallNormal = { 0, 1, 0 };
            hitEntity = testEntity;
          }

          // top
          if(TestWall(radiusH,
               entityDelta.y,
               entityDelta.x,
               Rel.y,
               Rel.x,
               radiusW,
               &tMin)) {
            WallNormal = { 0, -1, 0 };
            hitEntity = testEntity;
          }
        }
      }
    }

    entity->p += tMin * entityDelta;
    distanceRemaining -= tMin * entityDeltaLength;

    if(hitEntity) {
      entityDelta = targetEntityP - entity->p;

      bool32 stopsOnCollision = HandleCollision(entity, hitEntity);

      if(stopsOnCollision) {
        entity->dP
          = entity->dP - 1 * Inner(entity->dP, WallNormal) * WallNormal;
        entityDelta
          = entityDelta - 1 * Inner(entityDelta, WallNormal) * WallNormal;
      } else {
        AddCollisionRule(state, entity->stored, hitEntity->stored, false);
      }
    } else {
      break;
    }
  }

  if(entity->p.z < 0.0f) {
    entity->p.z = 0.0f;
    entity->dP.z = 0.0f;
  }

  if(AbsoluteValue(entity->dP.x) > AbsoluteValue(entity->dP.y)) {
    if(entity->dP.x > 0) {
      entity->facingDirection = 0;
    } else {
      entity->facingDirection = 2;
    }
  } else {
    if(entity->dP.y > 0) {
      entity->facingDirection = 1;
    } else {
      entity->facingDirection = 3;
    }
  }

  if(entity->distanceLimit != 0.0f) {
    entity->distanceLimit = distanceRemaining;
  }
}

#include <windows.h>
#include <stdio.h>

internal void
EndSim(sim_region *simRegion, game_state *state)
{
  for(uint32 index = 0; index < simRegion->entityCount; index++) {
    sim_entity *entity = simRegion->entities + index;
    stored_entity *stored = entity->stored;

    Assert(HasFlag(&stored->sim, EntityFlag_Simming));
    stored->sim = *entity;

    StoreEntityReference(&stored->sim.sword);

    world_position newP = HasFlag(entity, EntityFlag_NonSpatial)
      ? NullPosition()
      : MapIntoWorldSpace(simRegion->world, simRegion->origin, entity->p);

    ChangeEntityLocation(&state->worldArena, simRegion->world, stored, newP);

    if(stored == state->cameraFollowingEntity) {
      real32 oldZ = state->cameraP._offset.z;
      state->cameraP = newP;
      state->cameraP._offset.z = oldZ;
    }
  }
}
