#include "handmade.h"
#include "handmade_random.h"
#include "handmade_world.cpp"
#include "handmade_entity.cpp"
#include "handmade_sim_region.cpp"

internal sim_entity_collision_volume_group *
MakeSimpleCollision(game_state *state, real32 dimX, real32 dimY, real32 dimZ)
{
  memory_arena *arena = &state->worldArena;
  // TODO: Change to using types arena
  sim_entity_collision_volume_group *group
    = PushStruct(arena, sim_entity_collision_volume_group);
  group->volumeCount = 1;
  group->volumes
    = PushArray(arena, group->volumeCount, sim_entity_collision_volume);
  group->totalVolume.offset = v3{ 0, 0, 0.5f * dimZ };
  group->totalVolume.dim = v3{ dimX, dimY, dimZ };
  group->volumes[0] = group->totalVolume;

  return group;
}

internal sim_entity_collision_volume_group *
MakeNullCollision(game_state *state)
{
  memory_arena *arena = &state->worldArena;
  sim_entity_collision_volume_group *group
    = PushStruct(arena, sim_entity_collision_volume_group);
  group->volumeCount = 0;
  group->volumes = 0;
  group->totalVolume.offset = v3{ 0, 0, 0 };
  group->totalVolume.dim = v3{ 0, 0, 0 };
  return group;
}

internal stored_entity *
AddStoredEntity(game_state *state, entity_type type, world_position p)
{
  Assert(state->entityCount < ArrayCount(state->entities));
  stored_entity *stored = state->entities + state->entityCount++;
  stored->p = NullPosition();
  stored->sim.type = type;
  stored->sim.collision = state->nullCollision;

  ChangeEntityLocation(&state->worldArena, &state->world, stored, p);

  return stored;
}

internal stored_entity *
AddStoredEntity(game_state *state,
  entity_type type,
  int32 tileX,
  int32 tileY,
  int32 tileZ,
  v3 offset = {})
{
  world_position p
    = WorldPositionFromTilePosition(&state->world, tileX, tileY, tileZ, offset);
  return AddStoredEntity(state, type, p);
}

internal stored_entity *
AddGroundedEntity(game_state *state,
  entity_type type,
  world_position p,
  sim_entity_collision_volume_group *collision)
{
  stored_entity *entity = AddStoredEntity(state, type, p);
  entity->sim.collision = collision;
  return entity;
}

internal void
InitHitPoints(stored_entity *stored, int value)
{
  stored->sim.hitPointCount = value;

  for(int i = 0; i < value; i++) {
    stored->sim.hitPoints[i].amount = HIT_POINT_AMOUNT;
  }
}

internal stored_entity *
AddStandardRoom(game_state *state, int32 tileX, int32 tileY, int32 tileZ)
{
  world_position p
    = WorldPositionFromTilePosition(&state->world, tileX, tileY, tileZ);
  stored_entity *stored = AddGroundedEntity(state,
    EntityType_Space,
    p,
    state->standardRoomCollision);

  AddFlags(&stored->sim, EntityFlag_Traversable);

  return stored;
}

internal stored_entity *
AddWall(game_state *state, int32 tileX, int32 tileY, int32 tileZ)
{
  world_position p
    = WorldPositionFromTilePosition(&state->world, tileX, tileY, tileZ);
  stored_entity *stored
    = AddGroundedEntity(state, EntityType_Wall, p, state->wallCollision);

  return stored;
}

internal stored_entity *
AddSword(game_state *state)
{
  stored_entity *sword = AddGroundedEntity(state,
    EntityType_Sword,
    NullPosition(),
    state->swordCollision);

  AddFlags(&sword->sim, EntityFlag_Moveable);

  return sword;
}

internal stored_entity *
AddHero(game_state *state)
{
  stored_entity *hero = AddGroundedEntity(state,
    EntityType_Hero,
    state->cameraP,
    state->heroCollision);

  AddFlags(&hero->sim, EntityFlag_Moveable);

  InitHitPoints(hero, 3);

  hero->sim.facingDirection = 0;
  hero->sim.sword.stored = AddSword(state);

  return hero;
}

internal void
AddMonster(game_state *state, int32 tileX, int32 tileY, int32 tileZ)
{
  world_position p
    = WorldPositionFromTilePosition(&state->world, tileX, tileY, tileZ);
  stored_entity *monster
    = AddGroundedEntity(state, EntityType_Monster, p, state->monsterCollision);

  AddFlags(&monster->sim, EntityFlag_Moveable);

  InitHitPoints(monster, 3);
}

internal void
AddStairwell(game_state *state, int32 tileX, int32 tileY, int32 tileZ)
{
  world_position p
    = WorldPositionFromTilePosition(&state->world, tileX, tileY, tileZ);
  stored_entity *stored
    = AddGroundedEntity(state, EntityType_Stairwell, p, state->stairCollision);
  stored->sim.walkableDim = stored->sim.collision->totalVolume.dim.xy;
  stored->sim.walkableHeight = state->world.tileDepthInMeters;
}

internal void
AddFamiliar(game_state *state, int32 tileX, int32 tileY, int32 tileZ)
{
  world_position p
    = WorldPositionFromTilePosition(&state->world, tileX, tileY, tileZ);
  stored_entity *familiar = AddGroundedEntity(state,
    EntityType_Familiar,
    p,
    state->familiarCollision);

  AddFlags(&familiar->sim, EntityFlag_Moveable);
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
  uint32 horzResolution;
  uint32 vertResolution;
  uint32 colorsUsed;
  uint32 colorsImportant;
  uint32 redMask;
  uint32 greenMask;
  uint32 blueMask;
  uint32 alphaMask;
};
#pragma pack(pop)

inline uint8
ProcessPixelWithMask(uint32 Pixel, uint32 Mask)
{
  bit_scan_result result = FindLeastSignificantSetBit(Mask);
  Assert(result.found);
  return (uint8)(Pixel >> result.index);
}

// this is not a compelete BMP loading procedure
// we just need to load sutff from ourself's
// pixels from bottom to top
internal loaded_bitmap
LoadBMP(thread_context *thread,
  debug_platform_read_file ReadFile,
  char *fileName)
{
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
        bit_scan_result greenScan
          = FindLeastSignificantSetBit(header->greenMask);
        bit_scan_result blueScan = FindLeastSignificantSetBit(header->blueMask);
        bit_scan_result alphaScan
          = FindLeastSignificantSetBit(header->alphaMask);

        Assert(redScan.found);
        Assert(greenScan.found);
        Assert(blueScan.found);
        Assert(alphaScan.found);

        int32 redShift = 16 - (int32)redScan.index;
        int32 greenShift = 8 - (int32)greenScan.index;
        int32 blueShift = 0 - (int32)blueScan.index;
        int32 alphaShift = 24 - (int32)alphaScan.index;

        uint32 c = *pixel;
        *pixel++ = RotateLeft(c & header->redMask, redShift)
          | RotateLeft(c & header->greenMask, greenShift)
          | RotateLeft(c & header->blueMask, blueShift)
          | RotateLeft(c & header->alphaMask, alphaShift);
#endif
      }
    }
  }

  return result;
}

internal void
InitializeArena(memory_arena *arena, size_t size, void *base)
{
  arena->size = size;
  arena->base = (uint8 *)base;
  arena->used = 0;
}

internal void
DrawBitmap(game_offscreen_buffer *buffer,
  loaded_bitmap *bitmap,
  v2 minCorner,
  real32 cAlpha = 1.0f)
{
  int32 minX = RoundReal32ToInt32(minCorner.x);
  int32 minY = RoundReal32ToInt32(minCorner.y);
  int32 maxX = minX + bitmap->width;
  int32 maxY = minY + bitmap->height;

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

  uint32 *sourceRow = bitmap->pixel + clipY * bitmap->width + clipX;
  uint32 *destRow = (uint32 *)buffer->memory + minY * buffer->width + minX;

  for(int y = minY; y < maxY; y++) {
    uint32 *source = sourceRow;
    uint32 *dest = destRow;

    for(int x = minX; x < maxX; x++) {
      real32 alpha = (real32)((*source >> 24) & 0xff) / 255.0f;
      alpha *= cAlpha;

      real32 sR = (real32)((*source >> 16) & 0xff);
      real32 sG = (real32)((*source >> 8) & 0xff);
      real32 sB = (real32)((*source >> 0) & 0xff);

      real32 dR = (real32)((*dest >> 16) & 0xff);
      real32 dG = (real32)((*dest >> 8) & 0xff);
      real32 dB = (real32)((*dest >> 0) & 0xff);

      real32 r = (1 - alpha) * dR + alpha * sR;
      real32 g = (1 - alpha) * dG + alpha * sG;
      real32 b = (1 - alpha) * dB + alpha * sB;
      *dest = ((uint32)(r + 0.5f) << 16) | ((uint32)(g + 0.5f) << 8)
        | ((uint32)(b + 0.5f));

      dest++;
      source++;
    }

    sourceRow += bitmap->width;
    destRow += buffer->width;
  }
}

// exclusive
internal void
DrawRectangle(game_offscreen_buffer *buffer, v2 min, v2 max, v3 color)
{
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

  uint32 c = (RoundReal32ToUint32(color.r * 255.0f) << 16)
    | (RoundReal32ToUint32(color.g * 255.0f) << 8)
    | RoundReal32ToUint32(color.b * 255.0f);

  uint8 *row = (uint8 *)buffer->memory + minY * buffer->pitch
    + minX * buffer->bytesPerPixel;

  for(int y = minY; y < maxY; y++) {
    uint32 *Pixel = (uint32 *)row;
    for(int x = minX; x < maxX; x++) {
      *Pixel++ = c;
    }

    row += buffer->pitch;
  }
}

// `offset` is from the bottom-left
inline void
PushPiece(render_piece_group *group,
  loaded_bitmap *bitmap,
  v2 offset,
  real32 entityZC = 1.0f,
  real32 alpha = 1.0f)
{
  Assert(group->pieceCount < ArrayCount(group->pieces));
  render_piece piece = {};
  piece.bitmap = bitmap;
  piece.offset = offset;
  piece.alpha = alpha;
  piece.entityZC = entityZC;
  group->pieces[group->pieceCount++] = piece;
}

internal void
PushRect(render_piece_group *group, v2 offset, v2 halfDim, v3 color)
{
  Assert(group->pieceCount < ArrayCount(group->pieces));
  render_piece piece = {};
  piece.offset = offset;
  piece.color = color;
  piece.halfDim = halfDim;
  group->pieces[group->pieceCount++] = piece;
}

internal void
PushRectOutline(render_piece_group *group, v2 offset, v2 halfDim, v3 color)
{
  real32 thickness = 0.1f;
  // Top and bottom
  PushRect(group,
    v2{ offset.x, offset.y - halfDim.y },
    v2{ halfDim.x, thickness },
    v3{ 0.0f, 0, 1.0f });
  PushRect(group,
    v2{ offset.x, offset.y + halfDim.y },
    v2{ halfDim.x, thickness },
    v3{ 0.0f, 0, 1.0f });

  // Left and right
  PushRect(group,
    v2{ offset.x - halfDim.x, offset.y },
    v2{ thickness, halfDim.y },
    v3{ 0.0f, 0, 1.0f });
  PushRect(group,
    v2{ offset.x + halfDim.x, offset.y },
    v2{ thickness, halfDim.y },
    v3{ 0.0f, 0, 1.0f });
}

internal void
DrawHitPoints(sim_entity *entity, render_piece_group *pieceGroup)
{
  if(entity->hitPointCount > 0) {
    v2 halfDim = { 0.1f, 0.08f };
    real32 spanX = 1.5f * 2 * halfDim.x;
    real32 startX = -0.5f * (entity->hitPointCount - 1) * spanX;
    real32 startY = -0.75f * entity->collision->totalVolume.dim.y;
    for(uint32 hitPointIndex = 0; hitPointIndex < entity->hitPointCount;
        hitPointIndex++) {
      hit_point *hitPoint = entity->hitPoints + hitPointIndex;
      v3 color = v3{ 0.25f, 0.25f, 0.25f };
      if(hitPoint->amount > 0) {
        color = v3{ 1.0f, 0.0f, 0.0f };
      }
      PushRect(pieceGroup,
        v2{ startX + spanX * hitPointIndex, startY },
        halfDim,
        color);
    }
  }
}

internal void
InitializeWorld(game_world *world,
  real32 tileSizeInMeters,
  real32 tileDepthInMeters)
{
  world->tileSizeInMeters = tileSizeInMeters;
  world->tileDepthInMeters = tileDepthInMeters;
  world->chunkDimInMeters = {
    (real32)TILES_PER_CHUNK * tileSizeInMeters,
    (real32)TILES_PER_CHUNK * tileSizeInMeters,
    tileDepthInMeters,
  };
}

extern "C" GAME_UPDATE_VIDEO(GameUpdateVideo)
{
  Assert((&input->controllers[0].terminator - &input->controllers[0].buttons[0])
    == ArrayCount(input->controllers[0].buttons));
  Assert(sizeof(game_state) <= memory->permanentStorageSize);

  game_state *state = (game_state *)(memory->permanentStorage);
  memory_arena *worldArena = &state->worldArena;
  game_world *world = &state->world;

  int32 tilesPerWidth = 17;
  int32 tilesPerHeight = 9;

  if(!memory->isInitialized) {
    memory->isInitialized = true;

    state->background = LoadBMP(thread,
      memory->debugPlatformReadFile,
      "test/test_background.bmp");
    state->shadow = LoadBMP(thread,
      memory->debugPlatformReadFile,
      "test/test_hero_shadow.bmp");
    state->tree
      = LoadBMP(thread, memory->debugPlatformReadFile, "test2/tree00.bmp");
    state->sword
      = LoadBMP(thread, memory->debugPlatformReadFile, "test2/rock03.bmp");

    hero_bitmaps *heroBitmaps = state->heroBitmaps;
    heroBitmaps->offset = v2{ 72, 35 };
    heroBitmaps->head = LoadBMP(thread,
      memory->debugPlatformReadFile,
      "test/test_hero_right_head.bmp");
    heroBitmaps->cape = LoadBMP(thread,
      memory->debugPlatformReadFile,
      "test/test_hero_right_cape.bmp");
    heroBitmaps->torso = LoadBMP(thread,
      memory->debugPlatformReadFile,
      "test/test_hero_right_torso.bmp");

    heroBitmaps++;
    heroBitmaps->offset = v2{ 72, 35 };
    heroBitmaps->head = LoadBMP(thread,
      memory->debugPlatformReadFile,
      "test/test_hero_back_head.bmp");
    heroBitmaps->cape = LoadBMP(thread,
      memory->debugPlatformReadFile,
      "test/test_hero_back_cape.bmp");
    heroBitmaps->torso = LoadBMP(thread,
      memory->debugPlatformReadFile,
      "test/test_hero_back_torso.bmp");

    heroBitmaps++;
    heroBitmaps->offset = v2{ 72, 35 };
    heroBitmaps->head = LoadBMP(thread,
      memory->debugPlatformReadFile,
      "test/test_hero_left_head.bmp");
    heroBitmaps->cape = LoadBMP(thread,
      memory->debugPlatformReadFile,
      "test/test_hero_left_cape.bmp");
    heroBitmaps->torso = LoadBMP(thread,
      memory->debugPlatformReadFile,
      "test/test_hero_left_torso.bmp");

    heroBitmaps++;
    heroBitmaps->offset = v2{ 72, 35 };
    heroBitmaps->head = LoadBMP(thread,
      memory->debugPlatformReadFile,
      "test/test_hero_front_head.bmp");
    heroBitmaps->cape = LoadBMP(thread,
      memory->debugPlatformReadFile,
      "test/test_hero_front_cape.bmp");
    heroBitmaps->torso = LoadBMP(thread,
      memory->debugPlatformReadFile,
      "test/test_hero_front_torso.bmp");

    InitializeArena(worldArena,
      memory->permanentStorageSize - sizeof(game_state),
      (uint8 *)memory->permanentStorage + sizeof(game_state));

    InitializeWorld(world, 1.4f, 3.0f);

    state->wallCollision = MakeSimpleCollision(state,
      state->world.tileSizeInMeters,
      state->world.tileSizeInMeters,
      state->world.tileDepthInMeters);
    state->standardRoomCollision = MakeSimpleCollision(state,
      tilesPerWidth * state->world.tileSizeInMeters,
      tilesPerHeight * state->world.tileSizeInMeters,
      0.9f * state->world.tileDepthInMeters);
    state->heroCollision = MakeSimpleCollision(state, 1.0f, 0.5f, 1.2f);
    state->familiarCollision = MakeSimpleCollision(state, 1.0f, 0.5f, 0.5f);
    state->swordCollision = MakeSimpleCollision(state, 1.0f, 0.5f, 0.1f);
    state->monsterCollision = MakeSimpleCollision(state, 1.0f, 0.5f, 0.5f);
    state->stairCollision = MakeSimpleCollision(state,
      state->world.tileSizeInMeters,
      2.0f * state->world.tileSizeInMeters,
      1.1f * state->world.tileDepthInMeters);
    state->nullCollision = MakeNullCollision(state);

    int32 screenBaseX = 0;
    int32 screenBaseY = 0;
    int32 screenBaseZ = 0;
    int32 screenX = screenBaseX;
    int32 screenY = screenBaseY;

    int32 randomIndex = 0;
    bool32 doorLeft = false;
    bool32 doorRight = false;
    bool32 doorTop = false;
    bool32 doorBottom = false;
    bool32 doorUp = false;
    bool32 doorDown = false;
    int32 absTileZ = 0;

    for(int screenIndex = 0; screenIndex < 10; screenIndex++) {
      Assert(randomIndex < ArrayCount(randomNumberTable));
      int randomValue;

      // 0: left
      // 1: bottom
      // 2: up/down
      if(doorUp || doorDown) {
        randomValue = randomNumberTable[randomIndex++] % 2;
      } else {
        randomValue = randomNumberTable[randomIndex++] % 3;
      }

      // Make a stairwell in first screen
      if(screenIndex == 0) {
        randomValue = 2;
      }

      switch(randomValue) {
        case 0: {
          doorLeft = true;
        } break;

        case 1: {
          doorBottom = true;
        } break;

        case 2: {
          if(absTileZ == screenBaseZ) {
            doorUp = true;
          } else {
            doorDown = true;
          }
        } break;
      }

      AddStandardRoom(state,
        screenX * tilesPerWidth + tilesPerWidth / 2,
        screenY * tilesPerHeight + tilesPerHeight / 2,
        absTileZ);

      // 1: empty
      // 2: wall
      for(int32 tileY = 0; tileY < tilesPerHeight; tileY++) {
        for(int32 tileX = 0; tileX < tilesPerWidth; tileX++) {
          int32 absTileX = screenX * tilesPerWidth + tileX;
          int32 absTileY = screenY * tilesPerHeight + tileY;
          int32 TileValue = 1;

          if(tileX == 0) {
            TileValue = 2;
            if(doorLeft && tileY == (tilesPerHeight / 2)) {
              TileValue = 1;
            }
          }
          if(tileX == (tilesPerWidth - 1)) {
            TileValue = 2;
            if(doorRight && tileY == (tilesPerHeight / 2)) {
              TileValue = 1;
            }
          }
          if(tileY == 0) {
            TileValue = 2;
            if(doorBottom && tileX == (tilesPerWidth / 2)) {
              TileValue = 1;
            }
          }
          if(tileY == (tilesPerHeight - 1)) {
            TileValue = 2;
            if(doorTop && tileX == (tilesPerWidth / 2)) {
              TileValue = 1;
            }
          }

          if(TileValue == 2) {
            AddWall(state, absTileX, absTileY, absTileZ);
          }

          if(tileX == 10 && tileY == 4 && (doorUp || doorDown)) {
            AddStairwell(state,
              absTileX,
              absTileY,
              doorDown ? absTileZ - 1 : absTileZ);
          }
        }
      }

      doorRight = doorLeft;
      doorTop = doorBottom;
      doorLeft = false;
      doorBottom = false;

      if(randomValue == 0) {
        screenX--;
      } else if(randomValue == 1) {
        screenY--;
      } else if(randomValue == 2) {
        if(absTileZ == screenBaseZ) {
          doorDown = true;
          doorUp = false;
          absTileZ++;
        } else {
          doorUp = true;
          doorDown = false;
          absTileZ = screenBaseZ;
        }
      } else {
        doorDown = false;
        doorUp = false;
      }
    }

    world_position newCameraP = {};
    int32 cameraTileX = screenBaseX * tilesPerWidth + tilesPerWidth / 2;
    int32 cameraTileY = screenBaseY * tilesPerHeight + tilesPerHeight / 2;
    int32 cameraTileZ = 0;
    newCameraP = WorldPositionFromTilePosition(world,
      cameraTileX,
      cameraTileY,
      cameraTileZ);
    state->cameraP = newCameraP;

    AddMonster(state, cameraTileX - 2, cameraTileY + 2, cameraTileZ);

    // AddFamiliar(state, cameraTileX - 2, cameraTileY - 2, cameraTileZ);
  }

  real32 TileSizeInPixels = 60.0f;
  real32 metersToPixels = TileSizeInPixels / world->tileSizeInMeters;

  for(int controllerIndex = 0; controllerIndex < ArrayCount(input->controllers);
      controllerIndex++) {
    game_controller_input *controller = &input->controllers[controllerIndex];
    game_controller_input *oldController
      = &oldInput->controllers[controllerIndex];
    controlled_hero *conHero = state->controlledHeroes + controllerIndex;

    if(oldController->leftShoulder.isEndedDown == 0
      && controller->leftShoulder.isEndedDown != 0) {
      state->debugDrawBoundary = !state->debugDrawBoundary;
    }

    if(conHero->stored) {
      conHero->ddP = {};
      conHero->dSword = {};
      conHero->dZ = 0;

      if(controller->isAnalog) {
        conHero->ddP = { controller->stickAverageX, controller->stickAverageY };
      } else {
        if(controller->moveUp.isEndedDown) {
          conHero->ddP.y = 1.0f;
        }
        if(controller->moveDown.isEndedDown) {
          conHero->ddP.y = -1.0f;
        }
        if(controller->moveLeft.isEndedDown) {
          conHero->ddP.x = -1.0f;
        }
        if(controller->moveRight.isEndedDown) {
          conHero->ddP.x = 1.0f;
        }
      }

      if(controller->start.isEndedDown) {
        conHero->dZ = 3.0f;
      }

      if(controller->actionUp.isEndedDown) {
        conHero->dSword.y = 1.0f;
      }
      if(controller->actionDown.isEndedDown) {
        conHero->dSword.y = -1.0f;
      }
      if(controller->actionLeft.isEndedDown) {
        conHero->dSword.x = -1.0f;
      }
      if(controller->actionRight.isEndedDown) {
        conHero->dSword.x = 1.0f;
      }
    } else {
      if(controller->start.isEndedDown) {
        stored_entity *hero = AddHero(state);
        conHero->stored = hero;

        if(!state->cameraFollowingEntity) {
          state->cameraFollowingEntity = hero;
        }
      }
    }
  }

  int32 cameraSpanX = tilesPerWidth * 3;
  int32 cameraSpanY = tilesPerHeight * 3;
  int32 cameraSpanZ = 10;
  rectangle3 cameraBounds = RectCenterDim(v3{ 0, 0, 0 },
    v3{
      world->tileSizeInMeters * (real32)cameraSpanX,
      world->tileSizeInMeters * (real32)cameraSpanY,
      world->tileDepthInMeters * (real32)cameraSpanZ,
    });

  memory_arena simArena;
  InitializeArena(&simArena,
    memory->transientStorageSize,
    memory->transientStorage);
  sim_region *simRegion
    = BeginSim(&simArena, state, state->cameraP, cameraBounds, input->dt);

  //
  // Render
  //

  // Background
  DrawRectangle(buffer,
    v2{ 0, 0 },
    v2{ (real32)buffer->width, (real32)buffer->height },
    v3{ 0.5f, 0.5f, 0.5f });
  // DrawBitmap(buffer, state->background, 0, 0);

  v2 screenCenter = 0.5f * v2{ (real32)buffer->width, (real32)buffer->height };

  for(uint32 index = 0; index < simRegion->entityCount; index++) {
    sim_entity *entity = simRegion->entities + index;

    if(!entity->updatable) {
      continue;
    }

    render_piece_group pieceGroup = {};

    hero_bitmaps heroBitmaps = state->heroBitmaps[entity->facingDirection];

    move_spec moveSpec = {};
    v3 ddP = {};
    v3 oldSwordP = {};

    switch(entity->type) {
      case EntityType_Hero: {
        for(int conIndex = 0; conIndex < ArrayCount(state->controlledHeroes);
            conIndex++) {
          controlled_hero *conHero = state->controlledHeroes + conIndex;
          if(entity->stored == conHero->stored) {
            moveSpec = HeroMoveSpec();
            ddP = V3(conHero->ddP, 0.0f);

            if(conHero->dZ != 0.0f) {
              entity->dP.z = conHero->dZ;
            }

            if(conHero->dSword.x != 0.0f || conHero->dSword.y != 0.0f) {
              sim_entity *sword = entity->sword.entity;
              Assert(sword);

              if(HasFlag(sword, EntityFlag_NonSpatial)) {
                sword->distanceLimit = 5.0f;
                MakeEntitySpatial(sword, entity->p, 8.0f * conHero->dSword);
              }
            }
          }
        }

        real32 shadowAlpha = 1.0f - 0.5f * entity->p.z;
        if(shadowAlpha < 0.0f) {
          shadowAlpha = 0.0f;
        }

        PushPiece(&pieceGroup, &state->shadow, v2{ 70, 37 }, 0.0f, shadowAlpha);
        PushPiece(&pieceGroup, &heroBitmaps.cape, heroBitmaps.offset);
        PushPiece(&pieceGroup, &heroBitmaps.head, heroBitmaps.offset);
        DrawHitPoints(entity, &pieceGroup);
      } break;

      case EntityType_Sword: {
        moveSpec.ddPScale = 0.0f;
        moveSpec.drag = 0.0f;

        if(entity->distanceLimit <= 0.0f) {
          MakeEntityNonSpatial(entity);
          ClearCollisionRulesFor(state, entity->stored);
        }

        PushPiece(&pieceGroup, &state->sword, v2{ 28, 22 });
      } break;

      case EntityType_Wall: {
        PushPiece(&pieceGroup, &state->tree, v2{ 40, 40 }, 1);
      } break;

      case EntityType_Space: {
        for(uint32 volumeIndex = 0;
            volumeIndex < entity->collision->volumeCount;
            volumeIndex++) {
          sim_entity_collision_volume *volume
            = entity->collision->volumes + volumeIndex;
          PushRectOutline(&pieceGroup,
            v2{},
            0.5f * volume->dim.xy,
            v3{ 0.0f, 0, 1.0f });
        }
      } break;

      case EntityType_Monster: {
        PushPiece(&pieceGroup, &state->shadow, v2{ 70, 37 });
        PushPiece(&pieceGroup, &heroBitmaps.torso, heroBitmaps.offset);
        DrawHitPoints(entity, &pieceGroup);
      } break;

      case EntityType_Stairwell: {
        PushRect(&pieceGroup,
          v2{},
          0.5f * entity->walkableDim,
          v3{ 1.0f, 0, 0 });
      } break;

      case EntityType_Familiar: {
        sim_entity *closestHero = NULL;
        real32 closestDSq = Square(10.0f);

        for(uint32 i = 0; i < simRegion->entityCount; i++) {
          sim_entity *testEntity = simRegion->entities + i;
          if(testEntity->type == EntityType_Hero) {
            real32 testDSq = LengthSq(testEntity->p - entity->p);
            if(testDSq < closestDSq) {
              closestDSq = testDSq;
              closestHero = testEntity;
            }
          }
        }

        if(closestHero && (closestDSq >= Square(3.0f))) {
          ddP = closestHero->p - entity->p;
        }

        moveSpec = HeroMoveSpec();
        PushPiece(&pieceGroup, &state->shadow, v2{ 70, 37 }, 0.0f);
        PushPiece(&pieceGroup, &heroBitmaps.head, heroBitmaps.offset);
      } break;

      default: {
        InvalidCodePath;
      }
    }

    if(!HasFlag(entity, EntityFlag_NonSpatial)
      && HasFlag(entity, EntityFlag_Moveable)) {
      MoveEntity(state, simRegion, &moveSpec, entity, input->dt, ddP);
    }

    real32 entityZ = entity->p.z;
    real32 zFudge = 1.0f + 0.1f * entityZ;
    v2 entityCenter = screenCenter + zFudge * entity->p.xy * metersToPixels;

    if(state->debugDrawBoundary) {
      v2 entityOrigin = screenCenter + entity->p.xy * metersToPixels;
      v2 xy = entity->collision->totalVolume.dim.xy;
      DrawRectangle(buffer,
        entityOrigin - 0.5f * xy * metersToPixels,
        entityOrigin + 0.5f * xy * metersToPixels,
        v3{ 1.0f, 1.0f, 0.0f });
    }

    for(uint32 pieceIndex = 0; pieceIndex < pieceGroup.pieceCount;
        pieceIndex++) {
      render_piece *piece = pieceGroup.pieces + pieceIndex;
      if(piece->bitmap) {
        v2 minCorner = {
          entityCenter.x - piece->offset.x,
          entityCenter.y - piece->offset.y
            + entityZ * (piece->entityZC) * metersToPixels,
        };
        DrawBitmap(buffer, piece->bitmap, minCorner, piece->alpha);
      } else {
        DrawRectangle(buffer,
          entityCenter + metersToPixels * piece->offset
            - metersToPixels * piece->halfDim,
          entityCenter + metersToPixels * piece->offset
            + metersToPixels * piece->halfDim,
          piece->color);
      }
    }
  }

  // Debug: show current floors
  for(int i = 0; i < simRegion->origin.chunkZ + 1; i++) {
    v2 start = { 10 + (real32)i * 15, (real32)buffer->height - 20 };
    DrawRectangle(buffer, start, start + v2{ 10, 10 }, v3{ 0.0f, 1.0f, 0.0f });
  }

  EndSim(simRegion, state);
}

extern "C" GAME_UPDATE_AUDIO(GameUpdateAudio) {}
