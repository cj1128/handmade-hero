#include <stdio.h>
#include <windows.h>

internal bool32
TestWall(real32 wallX,
  real32 deltaX,
  real32 deltaY,
  real32 relX,
  real32 relY,
  real32 radiusY,
  real32 *tMin)
{
  real32 tEpsilon = 0.01f;
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

#include <stdio.h>

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
    AddFlags(&stored->sim, EntityFlag_Simming);

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

  for(int32 chunkZ = minChunk.chunkZ; chunkZ <= maxChunk.chunkZ; chunkZ++) {
    for(int32 chunkY = minChunk.chunkY; chunkY <= maxChunk.chunkY; chunkY++) {
      for(int32 chunkX = minChunk.chunkX; chunkX <= maxChunk.chunkX; chunkX++) {
        world_chunk *chunk
          = GetWorldChunk(simRegion->world, chunkX, chunkY, chunkZ);

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
  bool32 canCollide)
{
  if((uintptr_t)a > (uintptr_t)b) {
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
  found->canCollide = canCollide;
}

internal bool32
CanOverlap(sim_entity *mover, sim_entity *region)
{
  bool32 result = false;

  if(region->type == EntityType_Stairwell) {
    result = true;
  }

  return result;
}

internal bool32
CanCollide(game_state *state, sim_entity *a, sim_entity *b)
{
  bool32 result = false;

  if(a != b) {
    if(!HasFlag(a, EntityFlag_NonSpatial)
      && !HasFlag(b, EntityFlag_NonSpatial)) {
      result = true;
    }

    if((uintptr_t)(a->stored) > (uintptr_t)(b->stored)) {
      sim_entity *tmp = a;
      a = b;
      b = tmp;
    }

    uint32 hashBucket
      = (uintptr_t)(a->stored) & (ArrayCount(state->collisionRuleHash) - 1);
    for(pairwise_collision_rule *rule = state->collisionRuleHash[hashBucket];
        rule;
        rule = rule->nextInHash) {
      if(rule->a == a->stored && rule->b == b->stored) {
        result = rule->canCollide;
      }
    }
  }

  return result;
}

internal void
HandleOverlap(sim_entity *mover, sim_entity *region, real32 *ground, real32 dt)
{
  if(region->type == EntityType_Stairwell) {
    rectangle3 regionRect = RectCenterDim(region->p, region->dim);
    v3 bary = Clamp01(GetBarycentric(regionRect, mover->p));

    *ground = Lerp(bary.y, regionRect.min.z, regionRect.max.z);
  }
}

internal bool32
HandleCollision(game_state *state, sim_entity *a, sim_entity *b)
{
  bool32 stopsOnCollision = true;

  if(a->type > b->type) {
    sim_entity *tmp = a;
    a = b;
    b = tmp;
  }

  if(b->type == EntityType_Sword) {
    stopsOnCollision = false;
    AddCollisionRule(state, a->stored, b->stored, false);
  }

  if(a->type == EntityType_Monster && b->type == EntityType_Sword) {
    if(a->hitPointCount > 0) {
      a->hitPointCount--;
    }
  }

  return stopsOnCollision;
}

internal bool32
SpeculativeCollide(sim_entity *mover, sim_entity *region)
{
  bool32 result = true;

  if(region->type == EntityType_Stairwell) {
    rectangle3 regionRect = RectCenterDim(region->p, region->dim);
    v3 bary = Clamp01(GetBarycentric(regionRect, mover->p));
    real32 ground = Lerp(bary.y, regionRect.min.z, regionRect.max.z);

    result
      = (Abs(ground - mover->p.z) > 0.1f || (bary.y > 0.1f && bary.y < 0.9f));
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
  Assert(entity && !HasFlag(entity, EntityFlag_NonSpatial)
    && HasFlag(entity, EntityFlag_Moveable));

  if(entity->type == EntityType_Hero) {
    int foo = 0;
  }

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
  if(!HasFlag(entity, EntityFlag_ZSupported)) {
    ddP += v3{ 0, 0, -9.8f }; // Gravity
  }

  v3 entityDelta = 0.5f * ddP * Square(dt) + entity->dP * dt;
  entity->dP += ddP * dt;

  real32 distanceRemaining = entity->distanceLimit;

  if(distanceRemaining == 0.0f) {
    distanceRemaining = 1000.0f;
  }

  for(int32 iteration = 0; iteration < 4; iteration++) {
    real32 tMin = 1.0f;

    real32 entityDeltaLength = Length(entityDelta);

    if(entityDeltaLength == 0.0f) {
      break;
    }

    if(entityDeltaLength > distanceRemaining) {
      tMin = distanceRemaining / entityDeltaLength;
    }

    sim_entity *hitEntity = NULL;
    v3 wallNormal = {};
    v3 targetEntityP = entity->p + entityDelta;

    for(uint32 index = 0; index < simRegion->entityCount; index++) {
      sim_entity *testEntity = simRegion->entities + index;
      if(CanCollide(state, entity, testEntity)) {
        v3 rel = entity->p - testEntity->p;

        real32 radiusW = 0.5f * testEntity->dim.x + 0.5f * entity->dim.x;
        real32 radiusH = 0.5f * testEntity->dim.y + 0.5f * entity->dim.y;

        v3 testWallNormal = {};
        real32 testTmin = 1.0f;
        bool32 hitThis = false;

        // left
        if(TestWall(-radiusW,
             entityDelta.x,
             entityDelta.y,
             rel.x,
             rel.y,
             radiusH,
             &testTmin)) {
          testWallNormal = { 1, 0, 0 };
          hitThis = true;
        }

        // right
        if(TestWall(radiusW,
             entityDelta.x,
             entityDelta.y,
             rel.x,
             rel.y,
             radiusH,
             &testTmin)) {
          testWallNormal = { -1, 0, 0 };
          hitThis = true;
        }

        // bottom
        if(TestWall(-radiusH,
             entityDelta.y,
             entityDelta.x,
             rel.y,
             rel.x,
             radiusW,
             &testTmin)) {
          testWallNormal = { 0, 1, 0 };
          hitThis = true;
        }

        // top
        if(TestWall(radiusH,
             entityDelta.y,
             entityDelta.x,
             rel.y,
             rel.x,
             radiusW,
             &testTmin)) {
          testWallNormal = { 0, -1, 0 };
          hitThis = true;
        }

        if(hitThis) {
          if(SpeculativeCollide(entity, testEntity)) {
            tMin = testTmin;
            wallNormal = testWallNormal;
            hitEntity = testEntity;
          }
        }
      }
    }

    entity->p += tMin * entityDelta;
    distanceRemaining -= tMin * entityDeltaLength;

    if(hitEntity) {
      entityDelta = targetEntityP - entity->p;
      bool32 stopsOnCollision = HandleCollision(state, entity, hitEntity);

      if(stopsOnCollision) {
        entity->dP
          = entity->dP - 1 * Inner(entity->dP, wallNormal) * wallNormal;
        entityDelta
          = entityDelta - 1 * Inner(entityDelta, wallNormal) * wallNormal;
      }
    } else {
      break;
    }
  }

  real32 ground = 0;

  // Check overlapping
  rectangle3 entityRect = RectCenterDim(entity->p, entity->dim);
  for(uint32 testIndex = 0; testIndex < simRegion->entityCount; testIndex++) {
    sim_entity *testEntity = simRegion->entities + testIndex;

    if(testEntity == entity) {
      continue;
    }

    if(CanOverlap(entity, testEntity)) {
      rectangle3 testEntityRect = RectCenterDim(testEntity->p, testEntity->dim);
      if(RectanglesIntersect(entityRect, testEntityRect)) {
        HandleOverlap(entity, testEntity, &ground, dt);
      }
    }
  }

  if(entity->p.z <= ground
    || (HasFlag(entity, EntityFlag_ZSupported) && entity->dP.z == 0.0f)) {
    entity->p.z = ground;
    entity->dP.z = 0.0f;
    AddFlags(entity, EntityFlag_ZSupported);
  } else {
    ClearFlags(entity, EntityFlag_ZSupported);
  }

  if(Abs(entity->dP.x) > Abs(entity->dP.y)) {
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
