#ifndef HANDMADE_ENTITY_H
#define HANDMADE_ENTITY_H

#define INVALID_P (v2{1000, 1000})

#define HIT_POINT_AMOUNT 4
struct hit_point {
  uint8 flags;
  uint8 amount;
};

enum entity_type {
  EntityType_Null,

  EntityType_Hero,
  EntityType_Wall,
  EntityType_Monster,
  EntityType_Familiar,
  EntityType_Sword,
};

enum entity_flag {
  EntityFlag_Collides = (1 << 0),
  EntityFlag_NonSpatial = (1 << 1),

  // debug flags
  EntityFlag_Simming = (1 << 30),
};

struct stored_entity;
struct sim_entity;
union entity_reference {
  sim_entity *entity;
  stored_entity *stored;
};

struct sim_entity {
  stored_entity *stored;
  bool32 updatable;

  entity_type type;

  uint32 flags;

  v2 p;
  v2 dP;

  // 0: right, 1: up, 2: left, 3: down
  uint32 facingDirection;

  real32 width;
  real32 height;

  uint32 hitPointCount;
  hit_point hitPoints[16];

  real32 distanceRemaining;

  entity_reference sword;
};

struct stored_entity {
  world_position p;
  sim_entity sim;
};

inline bool32
HasFlag(sim_entity *entity, uint32 flag) {
  bool32 result = entity->flags & flag;
  return result;
}

inline void
AddFlag(sim_entity *entity, uint32 flag) {
  entity->flags |= flag;
}

inline void
ClearFlag(sim_entity *entity, uint32 flag) {
  entity->flags &= ~flag;
}

inline void
MakeEntityNonSpatial(sim_entity *entity) {
  AddFlag(entity, EntityFlag_NonSpatial);
  entity->p = INVALID_P;
}

inline void
MakeEntitySpatial(sim_entity *entity, v2 p, v2 dP) {
  ClearFlag(entity, EntityFlag_NonSpatial);
  entity->p = p;
  entity->dP = dP;
}

#endif
