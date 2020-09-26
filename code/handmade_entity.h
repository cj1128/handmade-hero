#ifndef HANDMADE_ENTITY_H
#define HANDMADE_ENTITY_H

#define INVALID_P (v3{ 1000, 1000, 1000 })

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
  EntityType_Stairwell,
};

enum entity_flag {
  EntityFlag_NonSpatial = (1 << 0),
  EntityFlag_Moveable = (1 << 1),
  EntityFlag_ZSupported = (1 << 2),

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

  v3 p;
  v3 dP;

  // 0: right, 1: up, 2: left, 3: down
  uint32 facingDirection;

  v3 dim;

  uint32 hitPointCount;
  hit_point hitPoints[16];

  // 0 means no limit
  real32 distanceLimit;

  entity_reference sword;

  // only for stairwell
  real32 walkableHeight;
};

struct stored_entity {
  world_position p;
  sim_entity sim;
};

inline bool32
HasFlag(sim_entity *entity, uint32 flag)
{
  bool32 result = entity->flags & flag;
  return result;
}

inline void
AddFlags(sim_entity *entity, uint32 flags)
{
  entity->flags |= flags;
}

inline void
ClearFlags(sim_entity *entity, uint32 flags)
{
  entity->flags &= ~flags;
}

inline void
MakeEntityNonSpatial(sim_entity *entity)
{
  AddFlags(entity, EntityFlag_NonSpatial);
  entity->p = INVALID_P;
}

inline void
MakeEntitySpatial(sim_entity *entity, v3 p, v3 dP)
{
  ClearFlags(entity, EntityFlag_NonSpatial);
  entity->p = p;
  entity->dP = dP;
}

internal v3
GetEntityGroundPoint(sim_entity *entity)
{
  v3 result = entity->p + v3{ 0, 0, -0.5f * entity->dim.z };
  return result;
}

internal real32
GetStairwellGround(sim_entity *entity, v3 moverGround)
{
  Assert(entity->type == EntityType_Stairwell);

  rectangle3 entityRect = RectCenterDim(entity->p, entity->dim);
  v3 bary = Clamp01(GetBarycentric(entityRect, moverGround));
  real32 ground = entityRect.min.z + bary.y * entity->walkableHeight;

  return ground;
}

#endif
