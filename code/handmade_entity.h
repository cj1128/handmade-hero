#ifndef HANDMADE_ENTITY_H
#define HANDMADE_ENTITY_H

#define INVALID_P (v3{ 1000, 1000, 1000 })

#define HIT_POINT_AMOUNT 4
struct hit_point {
  u8 flags;
  u8 amount;
};

enum entity_type {
  EntityType_Null,

  EntityType_Space,
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
  EntityFlag_Traversable = (1 << 3),

  // debug flags
  EntityFlag_Simming = (1 << 30),
};

struct stored_entity;
struct sim_entity;

union entity_reference {
  sim_entity *entity;
  stored_entity *stored;
};

struct sim_entity_collision_volume {
  v3 offset;
  v3 dim;
};

struct sim_entity_collision_volume_group {
  sim_entity_collision_volume totalVolume;

  // NOTE: currently volumeCount must > 0 if entity is collidable
  u32 volumeCount;
  sim_entity_collision_volume *volumes;
};

struct sim_entity {
  stored_entity *stored;
  bool32 updatable;

  entity_type type;

  u32 flags;

  v3 p; // ground point
  v3 dP;

  // 0: right, 1: up, 2: left, 3: down
  u32 facingDirection;

  sim_entity_collision_volume_group *collision;

  u32 hitPointCount;
  hit_point hitPoints[16];

  // 0 means no limit
  f32 distanceLimit;

  entity_reference sword;

  // only for stairwell
  f32 walkableHeight;
  v2 walkableDim;
};

struct stored_entity {
  world_position p;
  sim_entity sim;
};

inline bool32
HasFlag(sim_entity *entity, u32 flag)
{
  bool32 result = entity->flags & flag;
  return result;
}

inline void
AddFlags(sim_entity *entity, u32 flags)
{
  entity->flags |= flags;
}

inline void
ClearFlags(sim_entity *entity, u32 flags)
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
  v3 result = entity->p;
  return result;
}

internal f32
GetStairwellGround(sim_entity *entity, v3 moverGround)
{
  Assert(entity->type == EntityType_Stairwell);

  rectangle2 entityRect = RectCenterDim(entity->p.xy, entity->walkableDim);
  v2 bary = Clamp01(GetBarycentric(entityRect, moverGround.xy));
  f32 ground = entity->p.z + bary.y * entity->walkableHeight;

  return ground;
}

#endif
