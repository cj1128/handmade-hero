inline move_spec
HeroMoveSpec() {
  move_spec result = {};
  result.ddPScale = 80.0f;
  result.drag = 8.0f;
  result.unitddP = true;
  return result;
}

internal bool32
HandleCollision(sim_entity *a, sim_entity *b) {
  bool32 stopsOnCollision = true;

  if(a->type > b->type) {
    sim_entity *tmp = a;
    a = b;
    b = tmp;
  }

  if(b->type == EntityType_Sword) {
    stopsOnCollision = false;
  }

  if(a->type == EntityType_Monster && b->type == EntityType_Sword) {
    if(a->hitPointCount > 0) {
      a->hitPointCount--;
    }
  }

  return stopsOnCollision;
}
