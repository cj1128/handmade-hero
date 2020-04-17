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
  // TODO: this is ugly
  if(a->type == EntityType_Sword && b->type == EntityType_Monster) {
    if(b->hitPointCount > 0) {
      b->hitPointCount--;
    }
    return true;
  }

  if(a->type == EntityType_Monster && b->type == EntityType_Sword) {
    if(b->hitPointCount > 0) {
      b->hitPointCount--;
    }
    return true;
  }

  return false;
}
