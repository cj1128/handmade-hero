internal bool32
TestWall(
  real32 WallX,
  real32 PlayerDeltaX,
  real32 PlayerDeltaY,
  real32 RelX,
  real32 RelY,
  real32 RadiusY,
  real32 *tMin) {
  real32 tEpsilon = 0.001f;
  bool32 Hit = false;
  if(PlayerDeltaX != 0.0f) {
    real32 tResult = (WallX - RelX) / PlayerDeltaX;
    real32 y = RelY + tResult * PlayerDeltaY;
    if(y >= -RadiusY && y <= RadiusY) {
      if(tResult > 0.0f && tResult < *tMin) {
        Hit = true;
        *tMin = Maximum(0.0f, tResult - tEpsilon);
      }
    }
  }

  return Hit;
}

internal v2
GetSimSpaceP(sim_region *simRegion, stored_entity *entity) {
  v2 result = {};
  Assert(IsValid(&entity->p));

  world_diff diff =
    SubtractPosition(simRegion->world, entity->p, simRegion->origin);
  result = diff.dXY;

  return result;
}

internal sim_region_hash *
GetHashFromStored(sim_region *simRegion, stored_entity *stored) {
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

internal void
LoadEntityReference(sim_region *simRegion, entity_reference *ref);
internal sim_entity *
AddEntityToSimRegion(sim_region *simRegion, stored_entity *stored, v2 p) {
  Assert(stored);

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
StoreEntityReference(entity_reference *ref) {
  if(ref->entity) {
    ref->stored = ref->entity->stored;
  }
}

internal void
LoadEntityReference(sim_region *simRegion, entity_reference *ref) {
  if(ref->stored) {
    ref->entity = AddEntityToSimRegion(simRegion, ref->stored, INVALID_P);
  }
}

internal sim_region *
BeginSim(
  memory_arena *arena,
  game_state *state,
  world_position origin,
  rectangle2 bounds) {
  sim_region *simRegion = PushStruct(arena, sim_region);
  simRegion->entityCount = 0;
  ZeroStruct(simRegion->hash);

  simRegion->world = &state->world;
  simRegion->origin = origin;
  simRegion->bounds = bounds;

  world_position minChunk =
    MapIntoWorldSpace(simRegion->world, origin, bounds.min);
  world_position maxChunk =
    MapIntoWorldSpace(simRegion->world, origin, bounds.max);

  for(int32 chunkY = minChunk.chunkY; chunkY <= maxChunk.chunkY; chunkY++) {
    for(int32 chunkX = minChunk.chunkX; chunkX <= maxChunk.chunkX; chunkX++) {
      world_chunk *chunk =
        GetWorldChunk(simRegion->world, chunkX, chunkY, origin.chunkZ);

      if(chunk) {
        for(entity_block *block = &chunk->entityBlock; block;
            block = block->next) {
          for(uint32 index = 0; index < block->entityCount; index++) {
            stored_entity *stored = block->entities[index];
            Assert(stored);

            if(!HasFlag(&stored->sim, EntityFlag_NonSpatial)) {
              v2 simSpaceP = GetSimSpaceP(simRegion, stored);

              if(IsInRectangle(bounds, simSpaceP)) {
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
MoveEntity(
  sim_region *simRegion,
  move_spec *spec,
  sim_entity *entity,
  real32 dt,
  v2 ddP) {
  Assert(entity);

  game_world *world = simRegion->world;

  // Fix diagonal movement problem
  if(spec->unitddP) {
    real32 ddPLength = Length(ddP);
    if(ddPLength > 1.0f) {
      ddP *= (1.0f / ddPLength);
    }
  }

  ddP *= spec->ddPScale;
  ddP += -spec->drag * entity->dP;

  v2 entityDelta = 0.5f * ddP * Square(dt) + entity->dP * dt;
  v2 oldEntityP = entity->p;
  entity->dP += ddP * dt;

  if(HasFlag(entity, EntityFlag_Collides)) {
    for(int32 Iteration = 0; Iteration < 4; Iteration++) {
      real32 tMin = 1.0f;
      sim_entity *hitEntity = NULL;
      v2 WallNormal = {};
      v2 targetEntityP = oldEntityP + entityDelta;

      for(uint32 index = 0; index < simRegion->entityCount; index++) {
        sim_entity *testEntity = simRegion->entities + index;
        if(testEntity != entity) {
          if(HasFlag(testEntity, EntityFlag_Collides)) {
            v2 Rel = entity->p - testEntity->p;

            real32 radiusW = 0.5f * testEntity->width + 0.5f * entity->width;
            real32 radiusH = 0.5f * testEntity->height + 0.5f * entity->height;

            // left
            if(TestWall(
                 -radiusW,
                 entityDelta.x,
                 entityDelta.y,
                 Rel.x,
                 Rel.y,
                 radiusH,
                 &tMin)) {
              WallNormal = {1, 0};
              hitEntity = testEntity;
            }

            // right
            if(TestWall(
                 radiusW,
                 entityDelta.x,
                 entityDelta.y,
                 Rel.x,
                 Rel.y,
                 radiusH,
                 &tMin)) {
              WallNormal = {-1, 0};
              hitEntity = testEntity;
            }

            // bottom
            if(TestWall(
                 -radiusH,
                 entityDelta.y,
                 entityDelta.x,
                 Rel.y,
                 Rel.x,
                 radiusW,
                 &tMin)) {
              WallNormal = {0, 1};
              hitEntity = testEntity;
            }

            // top
            if(TestWall(
                 radiusH,
                 entityDelta.y,
                 entityDelta.x,
                 Rel.y,
                 Rel.x,
                 radiusW,
                 &tMin)) {
              WallNormal = {0, -1};
              hitEntity = testEntity;
            }
          }
        }
      }

      entity->p += tMin * entityDelta;
      if(hitEntity) {
        entity->dP =
          entity->dP - 1 * Inner(entity->dP, WallNormal) * WallNormal;
        entityDelta = targetEntityP - entity->p;
        entityDelta =
          entityDelta - 1 * Inner(entityDelta, WallNormal) * WallNormal;
        // entity->chunkZ += hitEntity->stored->dAbsTileZ;
      } else {
        break;
      }
    }
  } else {
    entity->p += entityDelta;
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
}

internal void
EndSim(sim_region *simRegion, game_state *state) {
  for(uint32 index = 0; index < simRegion->entityCount; index++) {
    sim_entity *entity = simRegion->entities + index;
    stored_entity *stored = entity->stored;

    Assert(HasFlag(&stored->sim, EntityFlag_Simming));
    stored->sim = *entity;

    StoreEntityReference(&stored->sim.sword);

    world_position newP =
      HasFlag(entity, EntityFlag_NonSpatial)
        ? NullPosition()
        : MapIntoWorldSpace(simRegion->world, simRegion->origin, entity->p);

    ChangeEntityLocation(&state->worldArena, simRegion->world, stored, newP);

    if(stored == state->cameraFollowingEntity) {
      state->cameraP = newP;
    }
  }
}
