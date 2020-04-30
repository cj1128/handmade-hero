#ifndef HANDMADE_SIM_REGION_H
#define HANDMADE_SIM_REGION_H

struct move_spec {
  bool32 unitddP;
  real32 ddPScale;
  real32 drag;
};

struct sim_region_hash {
  sim_entity *entity;
  stored_entity *stored;
};

struct sim_region {
  game_world *world;

  world_position origin;
  rectangle3 bounds;
  rectangle3 updatableBounds;

  uint32 entityCount;
  sim_entity entities[4096];

  sim_region_hash hash[4096];
};

#endif
