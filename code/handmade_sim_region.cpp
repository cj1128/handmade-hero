#include <stdio.h>
#include <windows.h>

struct test_wall {
  f32 x;
  f32 deltaX;
  f32 deltaY;
  f32 relX;
  f32 relY;
  f32 radiusY;
  v3 normal;
};

internal bool32
TestWall(f32 wallX,
  f32 deltaX,
  f32 deltaY,
  f32 relX,
  f32 relY,
  f32 radiusY,
  f32 *tMin)
{
  f32 tEpsilon = 0.001f;
  bool32 hit = false;

  if(deltaX != 0.0f) {
    f32 tResult = (wallX - relX) / deltaX;
    f32 y = relY + tResult * deltaY;
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

  u32 hashValue = (u32)((u8 *)stored - (u8 *)simRegion);

  for(int offset = 0; offset < ArrayCount(simRegion->hash); offset++) {
    u32 hashMask = ArrayCount(simRegion->hash) - 1;
    u32 hashIndex = (hashValue + offset) & hashMask;
    sim_region_hash *entry = simRegion->hash + hashIndex;

    if(entry->stored == NULL || entry->stored == stored) {
      result = entry;
      break;
    }
  }

  return result;
}

internal bool32
EntityOverlapsRectangle(v3 p,
  sim_entity_collision_volume volume,
  rectangle3 rect)
{
  rectangle3 grown = AddRadius(rect, 0.5f * volume.dim);
  bool32 result = IsInRectangle(grown, p + volume.offset);
  return result;
}

internal bool32
EntitiesOverlap(sim_entity *entity,
  sim_entity *testEntity,
  v3 epsilon = { 0, 0, 0 })
{
  bool32 result = false;

  for(u32 volumeIndex = 0;
      !result && (volumeIndex < entity->collision->volumeCount);
      volumeIndex++) {
    sim_entity_collision_volume *volume
      = entity->collision->volumes + volumeIndex;
    rectangle3 entityRect
      = RectCenterDim(entity->p + volume->offset, volume->dim + epsilon);

    for(u32 testVolumeIndex = 0;
        !result && (testVolumeIndex < testEntity->collision->volumeCount);
        testVolumeIndex++) {
      sim_entity_collision_volume *testVolume
        = testEntity->collision->volumes + testVolumeIndex;
      rectangle3 testEntityRect
        = RectCenterDim(testEntity->p + testVolume->offset, testVolume->dim);
      result = RectanglesIntersect(entityRect, testEntityRect);
    }
  }

  return result;
}

internal void LoadEntityReference(sim_region *simRegion, entity_reference *ref);
internal sim_entity *
AddEntityToSimRegion(sim_region *simRegion, stored_entity *stored, v3 p)
{
  {
    Assert(stored);
    sim_entity_collision_volume total = stored->sim.collision->totalVolume;
    // Assert(total.dim.x <= 2 * simRegion->maxEntityRadius);
    // Assert(total.dim.y <= 2 * simRegion->maxEntityRadius);
    // Assert(total.dim.z <= 2 * simRegion->maxEntityRadius);
  }

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
      entity->updatable = EntityOverlapsRectangle(p,
        entity->collision->totalVolume,
        simRegion->updatableBounds);
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
  f32 dt)
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

  f32 updateSafetyMargin
    = simRegion->maxEntityRadius + dt * simRegion->maxEntityVelocity;
  f32 updateSafetyMarginZ = 1.0f;
  simRegion->bounds = AddRadius(simRegion->updatableBounds,
    v3{ updateSafetyMargin, updateSafetyMargin, updateSafetyMarginZ });

  world_position minChunk = MapIntoWorldSpace(simRegion->world,
    origin,
    GetMinCorner(simRegion->bounds));
  world_position maxChunk = MapIntoWorldSpace(simRegion->world,
    origin,
    GetMaxCorner(simRegion->bounds));

  for(i32 chunkZ = minChunk.chunkZ; chunkZ <= maxChunk.chunkZ; chunkZ++) {
    for(i32 chunkY = minChunk.chunkY; chunkY <= maxChunk.chunkY; chunkY++) {
      for(i32 chunkX = minChunk.chunkX; chunkX <= maxChunk.chunkX; chunkX++) {
        world_chunk *chunk
          = GetWorldChunk(simRegion->world, chunkX, chunkY, chunkZ);

        if(chunk) {
          for(entity_block *block = &chunk->entityBlock; block;
              block = block->next) {
            for(u32 index = 0; index < block->entityCount; index++) {
              stored_entity *stored = block->entities[index];
              Assert(stored);

              if(!HasFlag(&stored->sim, EntityFlag_NonSpatial)) {
                v3 simSpaceP = GetSimSpaceP(simRegion, stored);

                if(EntityOverlapsRectangle(simSpaceP,
                     stored->sim.collision->totalVolume,
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
  for(u32 bucket = 0; bucket < ArrayCount(state->collisionRuleHash); bucket++) {
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
  u32 hashIndex = (uintptr_t)a & (ArrayCount(state->collisionRuleHash) - 1);
  for(pairwise_collision_rule *rule = state->collisionRuleHash[hashIndex]; rule;
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

    found->nextInHash = state->collisionRuleHash[hashIndex];
    state->collisionRuleHash[hashIndex] = found;
  }

  found->a = a;
  found->b = b;
  found->canCollide = canCollide;
}

internal bool32
CanOverlap(sim_entity *mover, sim_entity *region)
{
  bool32 result = false;

  if(mover->type != region->type) {
    if(region->type == EntityType_Stairwell) {
      result = true;
    }
  }

  return result;
}

internal bool32
CanCollide(game_state *state, sim_entity *a, sim_entity *b)
{
  bool32 result = false;

  if(a->type == EntityType_Space || b->type == EntityType_Space) {
    return false;
  }

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

    u32 hashBucket
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
HandleOverlap(sim_entity *mover, sim_entity *region, f32 *ground, f32 dt)
{
  if(region->type == EntityType_Stairwell) {
    *ground = GetStairwellGround(region, GetEntityGroundPoint(mover));
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
    v3 moverGround = GetEntityGroundPoint(mover);
    f32 ground = GetStairwellGround(region, moverGround);
    f32 stepHeight = 0.1f;

    // TODO: prevent the guy moving out of the stairwell
    result = Abs(ground - moverGround.z) > stepHeight;
  }

  return result;
}

internal void
MoveEntity(game_state *state,
  sim_region *simRegion,
  move_spec *spec,
  sim_entity *entity,
  f32 dt,
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
    f32 ddPLength = Length(ddP);
    if(ddPLength > 1.0f) {
      ddP *= (1.0f / ddPLength);
    }
  }

  Assert(LengthSq(entity->dP) <= Square(simRegion->maxEntityVelocity));

  ddP *= spec->ddPScale;
  v3 drag = -spec->drag * entity->dP;
  drag.z = 0;
  ddP += drag;
  if(!HasFlag(entity, EntityFlag_ZSupported)) {
    ddP += v3{ 0, 0, -9.8f }; // Gravity
  }

  v3 entityDelta = 0.5f * ddP * Square(dt) + entity->dP * dt;
  entity->dP += ddP * dt;

  Assert(entity->distanceLimit >= 0);
  f32 distanceRemaining = entity->distanceLimit;

  if(distanceRemaining == 0.0f) {
    distanceRemaining = 1000.0f;
  }

  for(i32 iteration = 0; iteration < 4; iteration++) {
    f32 tMin = 1.0f;
    f32 tMax = 0.0f;
    v3 wallNormalMin = {};
    v3 wallNormalMax = {};
    sim_entity *hitEntityMin = NULL;
    sim_entity *hitEntityMax = NULL;

    f32 entityDeltaLength = Length(entityDelta);

    if(entityDeltaLength == 0.0f) {
      break;
    }

    if(entityDeltaLength > distanceRemaining) {
      tMin = distanceRemaining / entityDeltaLength;
    }

    v3 targetEntityP = entity->p + entityDelta;

    for(u32 testEntityIndex = 0; testEntityIndex < simRegion->entityCount;
        testEntityIndex++) {
      sim_entity *testEntity = simRegion->entities + testEntityIndex;

      f32 overlapEpsilon = 0.1f;
      if((HasFlag(testEntity, EntityFlag_Traversable)
           && EntitiesOverlap(entity,
             testEntity,
             overlapEpsilon * v3{ 1, 1, 1 }))
        || CanCollide(state, entity, testEntity)) {
        for(u32 volumeIndex = 0; volumeIndex < entity->collision->volumeCount;
            volumeIndex++) {
          sim_entity_collision_volume *volume
            = entity->collision->volumes + volumeIndex;
          for(u32 testVolumeIndex = 0;
              testVolumeIndex < testEntity->collision->volumeCount;
              testVolumeIndex++) {
            sim_entity_collision_volume *testVolume
              = testEntity->collision->volumes + testVolumeIndex;

            v3 minkowskiDiameter = {
              volume->dim.x + testVolume->dim.x,
              volume->dim.y + testVolume->dim.y,
              volume->dim.z + testVolume->dim.z,
            };

            v3 rel = (entity->p + volume->offset)
              - (testEntity->p + testVolume->offset);

            v3 minCorner = -0.5f * minkowskiDiameter;
            v3 maxCorner = 0.5f * minkowskiDiameter;

            f32 radiusW = 0.5f * minkowskiDiameter.x;
            f32 radiusH = 0.5f * minkowskiDiameter.y;

            if(rel.z >= minCorner.z && rel.z < maxCorner.z) {
              test_wall testWalls[] = {
                // left
                {
                  minCorner.x,
                  entityDelta.x,
                  entityDelta.y,
                  rel.x,
                  rel.y,
                  radiusH,
                  { 1, 0, 0 },
                },
                // right
                { maxCorner.x,
                  entityDelta.x,
                  entityDelta.y,
                  rel.x,
                  rel.y,
                  radiusH,
                  { -1, 0, 0 } },
                // bottom
                { minCorner.y,
                  entityDelta.y,
                  entityDelta.x,
                  rel.y,
                  rel.x,
                  radiusW,
                  { 0, 1, 0 } },
                // top
                {
                  maxCorner.y,
                  entityDelta.y,
                  entityDelta.x,
                  rel.y,
                  rel.x,
                  radiusW,
                  { 0, -1, 0 },
                },
              };

              if(HasFlag(testEntity, EntityFlag_Traversable)) {
                v3 testWallNormal = {};
                f32 tMaxTest = tMax;
                bool32 hitThis = false;
                f32 tEpsilon = 0.001f;

                for(u32 wallIndex = 0; wallIndex < ArrayCount(testWalls);
                    wallIndex++) {
                  test_wall *wall = testWalls + wallIndex;
                  if(wall->deltaX != 0.0f) {
                    f32 tResult = (wall->x - wall->relX) / wall->deltaX;
                    f32 y = wall->relY + tResult * wall->deltaY;
                    if(y >= -wall->radiusY && y <= wall->radiusY) {
                      if(tResult > 0.0f && tResult > tMaxTest) {
                        hitThis = true;
                        testWallNormal = wall->normal;
                        tMaxTest = Maximum(0.0f, tResult - tEpsilon);
                      }
                    }
                  } else {
                    tMax = 1.0f;
                  }
                }

                if(hitThis) {
                  tMax = tMaxTest;
                  wallNormalMax = testWallNormal;
                  hitEntityMax = testEntity;
                }
              } else {
                v3 testWallNormal = {};
                f32 tMinTest = tMin;
                bool32 hitThis = false;
                f32 tEpsilon = 0.001f;

                for(u32 wallIndex = 0; wallIndex < ArrayCount(testWalls);
                    wallIndex++) {
                  test_wall *wall = testWalls + wallIndex;

                  if(wall->deltaX != 0.0f) {
                    f32 tResult = (wall->x - wall->relX) / wall->deltaX;
                    f32 y = wall->relY + tResult * wall->deltaY;
                    if(y >= -wall->radiusY && y <= wall->radiusY) {
                      if(tResult > 0.0f && tResult < tMinTest) {
                        hitThis = true;
                        testWallNormal = wall->normal;
                        tMinTest = Maximum(0.0f, tResult - tEpsilon);
                      }
                    }
                  }
                }

                if(hitThis) {
                  if(SpeculativeCollide(entity, testEntity)) {
                    tMin = tMinTest;
                    wallNormalMin = testWallNormal;
                    hitEntityMin = testEntity;
                  }
                }
              }
            }
          }
        }
      }
    }

    f32 tStop = 0.0f;
    v3 wallNormal = {};
    sim_entity *hitEntity = NULL;
    if(tMin < tMax) {
      tStop = tMin;
      wallNormal = wallNormalMin;
      hitEntity = hitEntityMin;
    } else {
      tStop = tMax;
      Assert(tMax <= 1.0f);
      wallNormal = wallNormalMax;
      hitEntity = hitEntityMax;
    }

    entity->p += tStop * entityDelta;
    distanceRemaining -= tStop * entityDeltaLength;

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

  f32 ground = 0;

  // Check overlapping
  for(u32 testIndex = 0; testIndex < simRegion->entityCount; testIndex++) {
    sim_entity *testEntity = simRegion->entities + testIndex;

    if(CanOverlap(entity, testEntity) && EntitiesOverlap(entity, testEntity)) {
      HandleOverlap(entity, testEntity, &ground, dt);
    }
  }

  ground += entity->p.z - GetEntityGroundPoint(entity).z;

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
  for(u32 index = 0; index < simRegion->entityCount; index++) {
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
      state->cameraP = newP;
    }
  }
}
