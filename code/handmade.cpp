#include "handmade.h"
#include "handmade_random.h"

internal high_entity *
MakeEntityHighFrequency(game_state *state, low_entity *low, v2 cameraSpaceP) {
  Assert(low->highEntity == NULL);
  game_world *world = &state->world;
  Assert(state->highEntityCount < ArrayCount(state->highEntities));

  high_entity *high = &state->highEntities[state->highEntityCount++];
  low->highEntity = high;
  high->lowEntity = low;
  high->p = cameraSpaceP;
  high->dP = {};
  high->chunkZ = low->p.chunkZ;
  high->facingDirection = 0;

  return high;
}

internal high_entity *
MakeEntityHighFrequency(game_state *state, low_entity *low) {
  if(low->highEntity) {
    return low->highEntity;
  }

  return MakeEntityHighFrequency(
    state,
    low,
    SubtractPosition(&state->world, low->p, state->cameraP).dXY
  );
}

// NOTE: This method modifies the state->highEntities array
internal void
MakeEntityLowFrequency(game_state *state, high_entity *high) {
  high->lowEntity->highEntity = NULL;

  high_entity *lastEntity = state->highEntities + state->highEntityCount - 1;
  if(high != lastEntity) {
    *high = *lastEntity;
    lastEntity->lowEntity->highEntity = high;
  }

  state->highEntityCount--;
}
internal void
ProcessEntityOutOfBound(
  game_state *state,
  v2 entityOffset,
  rectangle2 highFrequencyBound
) {
  for(uint32 entityIndex = 0;
    entityIndex < state->highEntityCount;
  ) {
    high_entity *high = state->highEntities + entityIndex;
    high->p += entityOffset;
    if(!IsInRectangle(highFrequencyBound, high->p) || !IsPositionValid(high->lowEntity->p)) {
      MakeEntityLowFrequency(state, high);
    } else {
      entityIndex++;
    }
  }
}

internal low_entity *
AddLowEntity(
  game_state *state,
  entity_type type,
  world_position *entityP
) {
  Assert(state->lowEntityCount < ArrayCount(state->lowEntities));
  state->lowEntityCount++;
  low_entity *entity = state->lowEntities + state->lowEntityCount - 1;
  entity->type = type;

  if(entityP) {
    entity->p = *entityP;
    ChangeEntityLocation(&state->memoryArena, &state->world, entity, NULL, entityP);
  } else {
    entity->p = NullPosition();
  }

  return entity;
}

internal low_entity *
AddLowEntity(
  game_state *state,
  entity_type type,
  int32 tileX,
  int32 tileY,
  int32 tileZ
) {
  world_position p = WorldPositionFromTilePosition(&state->world, tileX, tileY, tileZ);
  return AddLowEntity(state, type, &p);
}

internal low_entity *
AddWall(game_state *state, int32 tileX, int32 tileY, int32 tileZ) {
  low_entity *entity = AddLowEntity(state, EntityType_Wall, tileX, tileY, tileZ);
  entity->width = state->world.tileSizeInMeters;
  entity->height = entity->width;
  entity->collides = true;

  return entity;
}

internal void
InitHitPoints(low_entity *lowEntity, int value) {
  lowEntity->hitPointCount = value;
  for(int i = 0; i < value; i++) {
    lowEntity->hitPoints[i].amount = HIT_POINT_AMOUNT;
  }
}

internal low_entity *
AddSword(game_state *state) {
  low_entity *sword = AddLowEntity(state, EntityType_Sword, NULL);
  sword->width = 1.0f;
  sword->height = 0.5f;
  sword->collides = false;

  return sword;
}

internal high_entity *
AddHero(game_state *state) {
  low_entity *hero = AddLowEntity(state, EntityType_Hero, &state->cameraP);
  hero->height = 0.5f;
  hero->width = 1.0f;
  hero->collides = true;

  InitHitPoints(hero, 3);

  high_entity *highEntity = MakeEntityHighFrequency(state, hero);
  highEntity->facingDirection = 0;

  hero->sword = AddSword(state);

  return highEntity;
}

internal void
AddMonster(game_state *state, int32 tileX, int32 tileY, int32 tileZ) {
  low_entity *lowEntity = AddLowEntity(state, EntityType_Monster, tileX, tileY, tileZ);
  lowEntity->height = 0.5f;
  lowEntity->width = 1.0f;
  lowEntity->collides = true;
  InitHitPoints(lowEntity, 3);
}

internal void
AddFamiliar(game_state *state, int32 tileX, int32 tileY, int32 tileZ) {
  low_entity *lowEntity = AddLowEntity(state, EntityType_Familiar, tileX, tileY, tileZ);
  lowEntity->height = 0.5f;
  lowEntity->width = 1.0f;
  lowEntity->collides = false;
}

#pragma pack(push, 1)
struct bitmap_header {
  char signature[2];
  uint32 fileSize;
  uint32 reserved;
  uint32 dataOffset;

  uint32 size;
  int32 width;
  int32 height;
  uint16 planes;
  uint16 bitsPerPixel;
  uint32 compression;
  uint32 sizeOfBitmap;
  uint32  horzResolution;
  uint32  vertResolution;
  uint32 colorsUsed;
  uint32 colorsImportant;
  uint32 redMask;
  uint32 greenMask;
  uint32 blueMask;
  uint32 alphaMask;
};
#pragma pack(pop)

inline uint8
ProcessPixelWithMask(uint32 Pixel, uint32 Mask) {
  bit_scan_result result = FindLeastSignificantSetBit(Mask);
  Assert(result.found);
  return (uint8)(Pixel >> result.index);
}

// this is not a compelete BMP loading procedure
// we just need to load sutff from ourself's
// pixels from bottom to top
internal loaded_bitmap
LoadBMP(thread_context *thread, debug_platform_read_file ReadFile, char *fileName) {
  loaded_bitmap result = {};

  debug_read_file_result ReadResult = ReadFile(thread, fileName);
  if(ReadResult.memory) {
    bitmap_header *header = (bitmap_header *)ReadResult.memory;
    result.width = header->width;
    result.height = header->height;
    Assert(header->compression == 3);

    result.pixel = (uint32 *)((uint8 *)ReadResult.memory + header->dataOffset);

    // we have to restructure pixels
    // expected byte order: BB GG RR AA, 0xAARRGGBB
    uint32 *pixel = result.pixel;
    for(int y = 0; y < header->height; y++) {
      for(int x = 0; x < header->width; x++) {
#if 0
        uint8 A = ProcessPixelWithMask(*Pixel, header->alphaMask);
        uint8 r = ProcessPixelWithMask(*Pixel, header->redMask);
        uint8 g = ProcessPixelWithMask(*Pixel, header->greenMask);
        uint8 b = ProcessPixelWithMask(*Pixel, header->blueMask);
        *Pixel++ = (A << 24 | r << 16 | g << 8 | b);
#else
        bit_scan_result redScan = FindLeastSignificantSetBit(header->redMask);
        bit_scan_result greenScan = FindLeastSignificantSetBit(header->greenMask);
        bit_scan_result blueScan = FindLeastSignificantSetBit(header->blueMask);
        bit_scan_result alphaScan = FindLeastSignificantSetBit(header->alphaMask);

        Assert(redScan.found);
        Assert(greenScan.found);
        Assert(blueScan.found);
        Assert(alphaScan.found);

        int32 redShift = 16 - (int32)redScan.index;
        int32 greenShift = 8 - (int32)greenScan.index;
        int32 blueShift = 0 - (int32)blueScan.index;
        int32 alphaShift = 24 - (int32)alphaScan.index;

        uint32 c = *pixel;
        *pixel++ = RotateLeft(c & header->redMask, redShift) |
          RotateLeft(c & header->greenMask, greenShift) |
          RotateLeft(c & header->blueMask, blueShift) |
          RotateLeft(c & header->alphaMask, alphaShift);
#endif
      }
    }
  }

  return result;
}

internal void
InitializeArena(memory_arena *arena, size_t size, uint8 *base) {
  arena->size = size;
  arena->base = base;
  arena->used = 0;
}

internal void
DrawBitmap(
  game_offscreen_buffer *buffer,
  loaded_bitmap bitmap,
  v2 Position,
  v2 offset = v2{}
) {
  int32 minX = RoundReal32ToInt32(Position.x - offset.x);
  int32 minY = RoundReal32ToInt32(Position.y - offset.y);
  int32 maxX = minX + bitmap.width;
  int32 maxY = minY + bitmap.height;

  int32 clipX = 0;
  if(minX < 0) {
    clipX = -minX;
    minX = 0;
  }
  int32 clipY = 0;
  if(minY < 0) {
    clipY = -minY;
    minY = 0;
  }
  if(maxX > buffer->width) {
    maxX = buffer->width;
  }
  if(maxY > buffer->height) {
    maxY = buffer->height;
  }

  uint32 *sourceRow = bitmap.pixel + clipY*bitmap.width + clipX;
  uint32 *destRow = (uint32*)buffer->memory + minY*buffer->width + minX;

  for(int y = minY; y < maxY; y++) {
    uint32 *source = sourceRow;
    uint32 *dest = destRow;

    for(int x = minX; x < maxX; x++) {
      real32 sA = (real32)((*source >> 24) & 0xff);
      real32 sR = (real32)((*source >> 16) & 0xff);
      real32 sG = (real32)((*source >> 8) & 0xff);
      real32 sB = (real32)((*source >> 0) & 0xff);

      real32 dR = (real32)((*dest >> 16) & 0xff);
      real32 dG = (real32)((*dest >> 8) & 0xff);
      real32 dB = (real32)((*dest >> 0) &0xff);

      real32 ratio = sA / 255.0f;
      real32 r = (1 - ratio)*dR + ratio*sR;
      real32 g = (1 - ratio)*dG + ratio*sG;
      real32 b = (1 - ratio)*dB + ratio*sB;
      *dest = ((uint32)(r + 0.5f) << 16) | ((uint32)(g + 0.5f) << 8) | ((uint32)(b + 0.5f));

      dest++;
      source++;
    }

    sourceRow += bitmap.width;
    destRow += buffer->width;
  }
}

// exclusive
internal void
DrawRectangle(
  game_offscreen_buffer *buffer,
  v2 min,
  v2 max,
  v3 color
) {
  int32 minX = RoundReal32ToInt32(min.x);
  int32 minY = RoundReal32ToInt32(min.y);
  int32 maxX = RoundReal32ToInt32(max.x);
  int32 maxY = RoundReal32ToInt32(max.y);

  if(minX < 0) {
    minX = 0;
  }
  if(minY < 0) {
    minY = 0;
  }
  if(maxX > buffer->width) {
    maxX = buffer->width;
  }
  if(maxY > buffer->height) {
    maxY = buffer->height;
  }

  uint32 c = (RoundReal32ToUint32(color.r * 255.0f) << 16) |
  (RoundReal32ToUint32(color.g * 255.0f) << 8) |
  RoundReal32ToUint32(color.b * 255.0f);

  uint8 *row =  (uint8 *)buffer->memory + minY * buffer->pitch + minX * buffer->bytesPerPixel;

  for(int y = minY; y < maxY; y++) {
    uint32 *Pixel = (uint32 *)row;
    for(int x = minX; x < maxX; x++) {
      *Pixel++ = c;
    }

    row += buffer->pitch;
  }
}

internal void
SetCamera(game_state *state, world_position newCameraP) {
  game_world *world = &state->world;
  world_diff delta = SubtractPosition(world, newCameraP, state->cameraP);
  state->cameraP = newCameraP;
  v2 entityOffset = -delta.dXY;

  int32 tileSpanX = 17*4;
  int32 tileSpanY = 9*4;
  rectangle2 highFrequencyBound = RectCenterDim(v2{0, 0}, world->tileSizeInMeters * v2{(real32)tileSpanX, (real32)tileSpanY});

  ProcessEntityOutOfBound(state, entityOffset, highFrequencyBound);

  world_position minChunk = MapIntoWorldSpace(world, newCameraP, highFrequencyBound.min);
  world_position maxChunk = MapIntoWorldSpace(world, newCameraP, highFrequencyBound.max);

  for(int32 chunkY = minChunk.chunkY; chunkY < maxChunk.chunkY; chunkY++) {
    for(int32 chunkX = minChunk.chunkX; chunkX < maxChunk.chunkX; chunkX++) {
      world_chunk *chunk= GetWorldChunk(world, chunkX, chunkY, newCameraP.chunkZ);
      if(chunk) {
        entity_block *block = &chunk->entityBlock;
        for(; block; block = block->next) {
          for(
            uint32 entityIndex = 0;
            entityIndex < block->entityCount;
            entityIndex++
          ) {
            low_entity *low = block->lowEntities[entityIndex];
            Assert(low);
            if(!low->highEntity) {
              v2 cameraSpaceP = SubtractPosition(world, low->p, newCameraP).dXY;
              if(low->p.chunkZ == newCameraP.chunkZ &&
                IsInRectangle(highFrequencyBound, cameraSpaceP)) {
                MakeEntityHighFrequency(state, low, cameraSpaceP);
              }
            }
          }
        }
      }
    }
  }
}

internal bool32
TestWall(real32 WallX, real32 PlayerDeltaX, real32 PlayerDeltaY, real32 RelX, real32 RelY, real32 RadiusY, real32 *tMin) {
  real32 tEpsilon = 0.0001f;
  bool32 Hit = false;
  if(PlayerDeltaX != 0.0f) {
    real32 tResult = (WallX - RelX) / PlayerDeltaX;
    real32 y = RelY + tResult*PlayerDeltaY;
    if(y >= -RadiusY && y <= RadiusY) {
      if(tResult > 0.0f && tResult < *tMin) {
        Hit = true;
        *tMin = Maximum(0.0f, tResult - tEpsilon);
      }
    }
  }

  return Hit;
}

struct move_spec {
  real32 ddPScale;
  real32 drag;
};

inline move_spec
HeroMoveSpec() {
  move_spec result = {};
  result.ddPScale = 80.0f;
  result.drag = 8.0f;
  return result;
}

internal void
MoveEntity(move_spec *spec, game_state *state, high_entity *entity, real32 dt, v2 ddP) {
  Assert(entity);
  game_world *world = &state->world;

  ddP *= spec->ddPScale;
  ddP += -spec->drag * entity->dP;

  v2 entityDelta = 0.5f*ddP*Square(dt) + entity->dP*dt;
  v2 oldEntityP = entity->p;
  entity->dP += ddP*dt;

  if(entity->lowEntity->collides) {
    for(
      int32 Iteration = 0;
      Iteration < 4;
      Iteration++
    ) {
      real32 tMin = 1.0f;
      high_entity *HitEntity = 0;
      v2 WallNormal = {};
      v2 targetEntityP = oldEntityP + entityDelta;

      for(
        uint32 entityIndex = 0;
        entityIndex < state->highEntityCount;
        entityIndex++
      ) {
        high_entity* testEntity = state->highEntities + entityIndex;
        if(testEntity != entity) {
          if(testEntity->lowEntity->collides) {
            v2 Rel = entity->p - testEntity->p;

            real32 radiusW = 0.5f*testEntity->lowEntity->width + 0.5f*entity->lowEntity->width;
            real32 radiusH = 0.5f*testEntity->lowEntity->height + 0.5f*entity->lowEntity->height;

            // left
            if(TestWall(-radiusW, entityDelta.x, entityDelta.y, Rel.x, Rel.y, radiusH, &tMin)) {
              WallNormal = {1, 0};
              HitEntity = testEntity;
            }

            // right
            if(TestWall(radiusW, entityDelta.x, entityDelta.y, Rel.x, Rel.y, radiusH, &tMin)) {
              WallNormal = {-1, 0};
              HitEntity = testEntity;
            }

            // bottom
            if(TestWall(-radiusH, entityDelta.y, entityDelta.x, Rel.y, Rel.x, radiusW, &tMin)) {
              WallNormal = {0, 1};
              HitEntity = testEntity;
            }

            // top
            if(TestWall(radiusH, entityDelta.y, entityDelta.x, Rel.y, Rel.x, radiusW, &tMin)) {
              WallNormal = {0, -1};
              HitEntity = testEntity;
            }
          }
        }
      }

      entity->p += tMin*entityDelta;
      if(HitEntity) {
        entity->dP = entity->dP - 1*Inner(entity->dP, WallNormal)*WallNormal;
        entityDelta = targetEntityP - entity->p;
        entityDelta = entityDelta - 1*Inner(entityDelta, WallNormal)*WallNormal;
        // entity->chunkZ += HitEntity->lowEntity->dAbsTileZ;
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

  world_position newP = MapIntoWorldSpace(world, state->cameraP, entity->p);
  ChangeEntityLocation(
    &state->memoryArena, &state->world,
    entity->lowEntity,
    &entity->lowEntity->p, &newP
  );
  entity->lowEntity->p = newP;
}

inline void
PushPiece(render_piece_group *group, loaded_bitmap *bitmap, v2 offset) {
  Assert(group->pieceCount < ArrayCount(group->pieces));
  render_piece piece = {};
  piece.bitmap = bitmap;
  piece.offset = offset;
  group->pieces[group->pieceCount++] = piece;
}

internal void
UpdateFamiliar(game_state *state, high_entity *entity, real32 dt) {
  high_entity *ClosestHero = NULL;
  real32 ClosetDSq = Square(10.0f);

  for(uint32 entityIndex = 0; entityIndex < state->highEntityCount; entityIndex++) {
    high_entity *testEntity = state->highEntities + entityIndex;
    if(testEntity->lowEntity->type == EntityType_Hero) {
      real32 TestDSq = LengthSq(testEntity->p - entity->p);
      if(TestDSq < ClosetDSq) {
        ClosetDSq = TestDSq;
        ClosestHero = testEntity;
      }
    }
  }

  if(ClosestHero) {
    v2 ddP = ClosestHero->p - entity->p;

    move_spec moveSpec = HeroMoveSpec();
    MoveEntity(&moveSpec, state, entity, dt, 0.2f * ddP);
  }
}

internal void
PushRectangle(render_piece_group *group, v2 offset, v2 halfDim, v3 color) {
  Assert(group->pieceCount < ArrayCount(group->pieces));
  render_piece piece = {};
  piece.offset = offset;
  piece.color = color;
  piece.halfDim = halfDim;
  group->pieces[group->pieceCount++] = piece;
}

internal void
DarwHitPoints(low_entity *lowEntity, render_piece_group *pieceGroup) {
  if(lowEntity->hitPointCount > 0) {
    v2 halfDim = {0.2f, 0.1f};
    real32 spanX = 1.5f*2*halfDim.x;
    real32 startX = -0.5f*(lowEntity->hitPointCount - 1)*spanX;
    real32 startY = -0.75f * lowEntity->height;
    for(uint32 hitPointIndex = 0; hitPointIndex < lowEntity->hitPointCount; hitPointIndex++) {
      hit_point *hitPoint = lowEntity->hitPoints + hitPointIndex;
      v3 color = v3{0.25f, 0.25f, 0.25f};
      if(hitPoint->amount > 0) {
        color = v3{1.0f, 0.0f, 0.0f};
      }
      PushRectangle(pieceGroup, v2{startX + spanX*hitPointIndex, startY}, halfDim, color);
    }
  }
}

internal void
UpdateSword(game_state *state, high_entity *sword, real32 dt) {
  move_spec moveSpec = {};
  moveSpec.ddPScale = 0.0f;
  moveSpec.drag = 0.0f;

  v2 oldP = sword->p;
  MoveEntity(&moveSpec, state, sword, dt, v2{0, 0});
  real32 distance = Length(sword->p - oldP);
  sword->lowEntity->distanceRemaining -= distance;

  if(sword->lowEntity->distanceRemaining <= 0) {
    ChangeEntityLocation(
      &state->memoryArena,
      &state->world,
      sword->lowEntity,
      &sword->lowEntity->p, NULL
    );
    sword->lowEntity->p = NullPosition();
  }
}

extern "C" GAME_UPDATE_VIDEO(GameUpdateVideo) {
  Assert((&input->controllers[0].terminator - &input->controllers[0].buttons[0]) == ArrayCount(input->controllers[0].buttons))
  Assert(sizeof(game_state) <= memory->permanentStorageSize)

  game_state *state = (game_state *)(memory->permanentStorage);
  memory_arena *memoryArena = &state->memoryArena;
  game_world *world = &state->world;
  real32 TileSizeInPixels = 60.0f;
  world->tileSizeInMeters = 1.4f;
  real32 MetersToPixels = TileSizeInPixels / world->tileSizeInMeters;

  int32 TilesPerWidth = 17;
  int32 TilesPerHeight = 9;

  if(!memory->isInitialized) {
    memory->isInitialized = true;
    state->background = LoadBMP(thread, memory->debugPlatformReadFile, "test/test_background.bmp");
    state->tree = LoadBMP(thread, memory->debugPlatformReadFile, "test2/tree00.bmp");
    state->sword = LoadBMP(thread, memory->debugPlatformReadFile, "test2/rock03.bmp");

    hero_bitmaps *heroBitmaps = &state->heroBitmaps[0];
    heroBitmaps->offset = v2{72, 35};
    heroBitmaps->head = LoadBMP(thread, memory->debugPlatformReadFile, "test/test_hero_right_head.bmp");
    heroBitmaps->cape = LoadBMP(thread, memory->debugPlatformReadFile, "test/test_hero_right_cape.bmp");
    heroBitmaps->torso = LoadBMP(thread, memory->debugPlatformReadFile, "test/test_hero_right_torso.bmp");
    heroBitmaps++;

    heroBitmaps->offset = v2{72, 35};
    heroBitmaps->head = LoadBMP(thread, memory->debugPlatformReadFile, "test/test_hero_back_head.bmp");
    heroBitmaps->cape = LoadBMP(thread, memory->debugPlatformReadFile, "test/test_hero_back_cape.bmp");
    heroBitmaps->torso = LoadBMP(thread, memory->debugPlatformReadFile, "test/test_hero_back_torso.bmp");
    heroBitmaps++;

    heroBitmaps->offset = v2{72, 35};
    heroBitmaps->head = LoadBMP(thread, memory->debugPlatformReadFile, "test/test_hero_left_head.bmp");
    heroBitmaps->cape = LoadBMP(thread, memory->debugPlatformReadFile, "test/test_hero_left_cape.bmp");
    heroBitmaps->torso = LoadBMP(thread, memory->debugPlatformReadFile, "test/test_hero_left_torso.bmp");
    heroBitmaps++;

    heroBitmaps->offset = v2{72, 35};
    heroBitmaps->head = LoadBMP(thread, memory->debugPlatformReadFile, "test/test_hero_front_head.bmp");
    heroBitmaps->cape = LoadBMP(thread, memory->debugPlatformReadFile, "test/test_hero_front_cape.bmp");
    heroBitmaps->torso = LoadBMP(thread, memory->debugPlatformReadFile, "test/test_hero_front_torso.bmp");

    InitializeArena(memoryArena, memory->permanentStorageSize - sizeof(game_state), (uint8 *)memory->permanentStorage + sizeof(game_state));

    world->chunkSizeInMeters = TILES_PER_CHUNK * world->tileSizeInMeters;

    uint32 ScreenBaseX = 0;
    uint32 ScreenBaseY = 0;
    uint32 ScreenX = ScreenBaseX;
    uint32 ScreenY = ScreenBaseY;
    uint32 RandomIndex = 0;
    bool32 DoorLeft = false;
    bool32 DoorRight = false;
    bool32 DoorTop = false;
    bool32 DoorBottom = false;
    bool32 DoorUp = false;
    bool32 DoorDown = false;
    uint32 tileZ = 0;

    for(uint32 ScreenIndex = 0; ScreenIndex < 10; ScreenIndex++) {
      Assert(RandomIndex < ArrayCount(randomNumberTable));
      int RandomValue;

      // 0: left
      // 1: bottom
      // 2: up/down
      if(1) { // (DoorUp || DoorDown) {
        RandomValue = randomNumberTable[RandomIndex++] % 2;
      } else {
        RandomValue = randomNumberTable[RandomIndex++] % 3;
      }

      switch(RandomValue) {
        case 0: {
          DoorLeft = true;
        } break;

        case 1: {
          DoorBottom = true;
        } break;

        case 2:
          if(tileZ == 0) {
            DoorUp = true;
          } else if(tileZ == 1) {
            DoorDown = true;
          }
          break;
      }

      for(int32 tileY = 0; tileY < TilesPerHeight; tileY++) {
        for(int32 tileX = 0; tileX < TilesPerWidth; tileX++) {
          int32 absTileX = ScreenX*TilesPerWidth + tileX;
          int32 absTileY = ScreenY*TilesPerHeight + tileY;
          int32 TileValue = 1;

          if(tileX == 0) {
            TileValue = 2;
            if(DoorLeft && tileY == (TilesPerHeight / 2)) {
              TileValue = 1;
            }
          }
          if(tileX == (TilesPerWidth - 1)) {
            TileValue = 2;
            if(DoorRight && tileY == (TilesPerHeight / 2)) {
              TileValue = 1;
            }
          }
          if(tileY == 0) {
            TileValue = 2;
            if(DoorBottom && tileX == (TilesPerWidth / 2)) {
              TileValue = 1;
            }
          }
          if(tileY == (TilesPerHeight - 1)) {
            TileValue = 2;
            if(DoorTop && tileX == (TilesPerWidth / 2)) {
              TileValue = 1;
            }
          }

          if(tileX == 10 && tileY == 4 && (DoorUp || DoorDown)) {
            TileValue = 3;
          }

          if(TileValue == 2) {
            AddWall(state, absTileX, absTileY, tileZ);
          }
        }
      }

      DoorRight = DoorLeft;
      DoorTop = DoorBottom;
      DoorLeft = false;
      DoorBottom = false;

      if(RandomValue == 0) {
        ScreenX--;
      } else if(RandomValue == 1) {
        ScreenY--;
      } else if(RandomValue == 2) {
        if(tileZ == 0) {
          DoorDown = true;
          DoorUp = false;
          tileZ = 1;
        } else {
          DoorUp = true;
          DoorDown = false;
          tileZ = 0;
        }
      } else {
        DoorDown = false;
        DoorUp = false;
      }
    }

    world_position newCameraP = {};
    newCameraP = WorldPositionFromTilePosition(world, 8, 4, 0);
    AddMonster(state, 10, 6, 0);
    AddFamiliar(state, 6, 6, 0);
    SetCamera(state, newCameraP);
  }

#define TILE_COUNT_X 256
#define TILE_COUNT_Y 256
#define tile_chunk_COUNT_X 2
#define tile_chunk_COUNT_Y 2
  v2 ScreenCenter = {(real32)buffer->width / 2, (real32)buffer->height / 2};

  for(
    int controllerIndex = 0;
    controllerIndex < ArrayCount(input->controllers);
    controllerIndex++
  ) {
    game_controller_input *Controller = &input->controllers[controllerIndex];
    low_entity *lowEntity =  state->playerEntityForController[controllerIndex];
    if(lowEntity) {
      v2 ddP = {};

      if(Controller->isAnalog) {
        ddP = {Controller->stickAverageX, Controller->stickAverageY};
      } else {
        if(Controller->moveUp.isEndedDown) {
          ddP.y = 1.0f;
        }
        if(Controller->moveDown.isEndedDown) {
          ddP.y = -1.0f;
        }
        if(Controller->moveLeft.isEndedDown) {
          ddP.x = -1.0f;
        }
        if(Controller->moveRight.isEndedDown) {
          ddP.x = 1.0f;
        }
      }

      move_spec moveSpec = HeroMoveSpec();
      MoveEntity(&moveSpec, state, lowEntity->highEntity, input->dt, ddP);

      v2 dSword = {};
      if(Controller->actionUp.isEndedDown) {
        dSword.y = 1.0f;
      }
      if(Controller->actionDown.isEndedDown) {
        dSword.y = -1.0f;
      }
      if(Controller->actionLeft.isEndedDown) {
        dSword.x = -1.0f;
      }
      if(Controller->actionRight.isEndedDown) {
        dSword.x = 1.0f;
      }

      if(dSword.x != 0.0f || dSword.y != 0.0f) {
        low_entity *sword = lowEntity->sword;
        if(sword && !IsPositionValid(sword->p)) {
          sword->p = lowEntity->p;
          ChangeEntityLocation(&state->memoryArena, &state->world, sword, NULL, &sword->p);
          MakeEntityHighFrequency(state, sword);
          sword->distanceRemaining = 5.0f;
          sword->highEntity->dP = 8.0f*dSword;
        }
      }
    } else {
      if(Controller->start.isEndedDown) {
        high_entity *playerEntity = AddHero(state);
        state->playerEntityForController[controllerIndex] = playerEntity->lowEntity;

        if(!state->cameraFollowingEntity) {
          state->cameraFollowingEntity = playerEntity->lowEntity;
        }
      }
    }
  }

  low_entity *cameraFollowingEntity = state->cameraFollowingEntity;
  if(cameraFollowingEntity) {
    bool32 HasChanged = false;
    world_position newCameraP = state->cameraP;

    if(cameraFollowingEntity->highEntity->chunkZ != newCameraP.chunkZ) {
      newCameraP.chunkZ = cameraFollowingEntity->highEntity->chunkZ;
      HasChanged = true;
    }
#if 0
    v2 Diff = cameraFollowingEntity->highEntity->p;
    real32 ScreenWidth = TilesPerWidth * world->tileSizeInMeters;
    real32 ScreenHeight = TilesPerHeight * world->tileSizeInMeters;
    if(Diff.x > 0.5f*ScreenWidth) {
      HasChanged = true;
      newCameraP = MapIntoWorldSpace(world, newCameraP, v2{ScreenWidth, 0});
    }
    if(Diff.x < -0.5f*ScreenWidth){
      HasChanged = true;
      newCameraP = MapIntoWorldSpace(world, newCameraP, v2{-ScreenWidth, 0});
    }
    if(Diff.y > 0.5f*ScreenHeight) {
      HasChanged = true;
      newCameraP = MapIntoWorldSpace(world, newCameraP, v2{0, ScreenHeight});
    }
    if(Diff.y < -0.5f*ScreenHeight) {
      HasChanged = true;
      newCameraP = MapIntoWorldSpace(world, newCameraP, v2{0, -ScreenHeight});
    }
#else
    HasChanged = true;
    newCameraP = cameraFollowingEntity->p;
#endif
    if(HasChanged) {
      SetCamera(state, newCameraP);
    }
  }

  DrawRectangle(
    buffer,
    v2{0, 0}, v2{(real32)buffer->width, (real32)buffer->height},
    v3{0.5f, 0.5f, 0.5f}
  );
  // DrawBitmap(buffer, state->background, 0, 0);

  for(
    uint32 entityIndex = 0;
    entityIndex < state->highEntityCount;
    entityIndex++
  ) {
    high_entity *entity = state->highEntities + entityIndex;
    low_entity *lowEntity = entity->lowEntity;
    hero_bitmaps heroBitmaps = state->heroBitmaps[entity->facingDirection];
    render_piece_group pieceGroup = {};

    switch(lowEntity->type) {
      case EntityType_Hero: {
        PushPiece(&pieceGroup, &heroBitmaps.cape, heroBitmaps.offset);
        PushPiece(&pieceGroup, &heroBitmaps.head, heroBitmaps.offset);
        DarwHitPoints(lowEntity, &pieceGroup);
      } break;

      case EntityType_Sword: {
        UpdateSword(state, entity, input->dt);
        PushPiece(&pieceGroup, &state->sword, v2{28, 22});
      } break;

      case EntityType_Wall: {
        PushPiece(&pieceGroup, &state->tree, v2{40, 40});
      } break;

      case EntityType_Monster: {
        PushPiece(&pieceGroup, &heroBitmaps.torso, heroBitmaps.offset);
        DarwHitPoints(lowEntity, &pieceGroup);
      } break;

      case EntityType_Familiar: {
        UpdateFamiliar(state, entity, input->dt);
        PushPiece(&pieceGroup, &heroBitmaps.head, heroBitmaps.offset);
      } break;

      default: {
        InvalidCodePath;
      }
    }

    v2 entityCenter = ScreenCenter + entity->p*MetersToPixels;
    v2 EntityMin = entityCenter + v2{-0.5f*lowEntity->width * MetersToPixels, -0.5f*lowEntity->height * MetersToPixels};
    v2 EntityMax = entityCenter + v2{0.5f*lowEntity->width * MetersToPixels, 0.5f*lowEntity->height * MetersToPixels};

    DrawRectangle(buffer, EntityMin, EntityMax, v3{1.0f, 1.0f, 0.0f});

    for(uint32 PieceIndex = 0; PieceIndex < pieceGroup.pieceCount; PieceIndex++) {
      render_piece *piece = pieceGroup.pieces + PieceIndex;
      if(piece->bitmap) {
        DrawBitmap(buffer, *piece->bitmap, entityCenter, piece->offset);
      } else {
        DrawRectangle(
          buffer,
          entityCenter + MetersToPixels*piece->offset - MetersToPixels*piece->halfDim,
          entityCenter + MetersToPixels*piece->offset + MetersToPixels*piece->halfDim,
          piece->color
        );
      }
    }
  }
}

extern "C" GAME_UPDATE_AUDIO(GameUpdateAudio) {
}
