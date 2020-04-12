inline move_spec
HeroMoveSpec() {
  move_spec result = {};
  result.ddPScale = 80.0f;
  result.drag = 8.0f;
  result.unitddP = true;
  return result;
}

internal void
UpdateFamiliar(sim_region *simRegion, sim_entity *entity, real32 dt) {
  sim_entity *closestHero = NULL;
  real32 closestDSq = Square(10.0f);

  for(uint32 index = 0; index < simRegion->entityCount; index++) {
    sim_entity *testEntity = simRegion->entities + index;
    if(testEntity->type == EntityType_Hero) {
      real32 testDSq = LengthSq(testEntity->p - entity->p);
      if(testDSq < closestDSq) {
        closestDSq = testDSq;
        closestHero = testEntity;
      }
    }
  }

  if(closestHero) {
    v2 ddP = closestHero->p - entity->p;
    move_spec moveSpec = HeroMoveSpec();
    MoveEntity(simRegion, &moveSpec, entity, dt, 0.2f * ddP);
  }
}

internal void
UpdateSword(sim_region *simRegion, sim_entity *sword, real32 dt) {
  if(HasFlag(sword, EntityFlag_NonSpatial)) {
    return;
  }

  move_spec moveSpec = {};
  moveSpec.ddPScale = 0.0f;
  moveSpec.drag = 0.0f;

  v2 oldP = sword->p;
  MoveEntity(simRegion, &moveSpec, sword, dt, v2{0, 0});
  real32 distance = Length(sword->p - oldP);
  sword->distanceRemaining -= distance;

  if(sword->distanceRemaining <= 0) {
    MakeEntityNonSpatial(sword);
  }
}
