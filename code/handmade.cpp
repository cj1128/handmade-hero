#include "handmade.h"
#include "handmade_random.h"

internal high_entity *
MakeEntityHighFrequency(game_state *State, low_entity *Low) {
  if(Low->HighEntity) {
    return Low->HighEntity;
  }

  world *World = &State->World;
  Assert(State->HighEntityCount < ArrayCount(State->HighEntities));
  high_entity *High = &State->HighEntities[State->HighEntityCount++];
  Low->HighEntity = High;
  High->LowEntity = Low;

  world_diff Diff = SubtractPosition(World, Low->P, State->CameraP);
  High->P = Diff.dXY;
  High->dP = {};
  High->AbsTileZ = Low->P.AbsTileZ;
  High->FacingDirection = 0;

  return High;
}

// NOTE: This method modifies the State->HighEntities array
internal void
MakeEntityLowFrequency(game_state *State, high_entity *High) {
  High->LowEntity->HighEntity = 0;

  high_entity *LastEntity = State->HighEntities + State->HighEntityCount - 1;
  if(High != LastEntity) {
    *High = *LastEntity;
    LastEntity->LowEntity->HighEntity = High;
  }

  State->HighEntityCount--;
}
internal void
ProcessEntitiesOutsideOfCamera(game_state *State, v2 EntityOffset, rectangle2 HighFrequencyBound) {
  for(uint32 EntityIndex = 0;
    EntityIndex < State->HighEntityCount;
  ) {
    high_entity *High = State->HighEntities + EntityIndex;
    High->P += EntityOffset;
    if(!IsInRectangle(HighFrequencyBound, High->P)) {
      MakeEntityLowFrequency(State, High);
    } else {
      EntityIndex++;
    }
  }
}

internal low_entity*
AddLowEntity(game_state *State, entity_type Type) {
  Assert(State->LowEntityCount < ArrayCount(State->LowEntities));
  State->LowEntityCount++;
  low_entity *Entity = State->LowEntities + State->LowEntityCount - 1;
  Entity->Type = Type;
  return Entity;
}

internal low_entity *
AddWall(game_state *State, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
  low_entity *Entity = AddLowEntity(State, EntityType_Wall);
  Entity->P.AbsTileX = AbsTileX;
  Entity->P.AbsTileY = AbsTileY;
  Entity->P.AbsTileZ = AbsTileZ;
  Entity->Width = State->World.TileSizeInMeters;
  Entity->Width = State->World.TileSizeInMeters;
  Entity->Height = Entity->Width;
  Entity->Collides = true;

  return Entity;
}

internal high_entity *
AddPlayer(game_state *State) {
  low_entity *LowEntity = AddLowEntity(State, EntityType_Hero);

  LowEntity->P.AbsTileX = 2;
  LowEntity->P.AbsTileY = 3;
  LowEntity->P.AbsTileZ = 0;
  LowEntity->Height = 0.5f;
  LowEntity->Width = 1.0f;
  LowEntity->Collides = true;

  high_entity *HighEntity = MakeEntityHighFrequency(State, LowEntity);
  HighEntity->FacingDirection = 0;

  return HighEntity;
}

#pragma pack(push, 1)
struct bitmap_header {
  char Signature[2];
  uint32 FileSize;
  uint32 Reserved;
  uint32 DataOffset;

  uint32 Size;
  int32 Width;
  int32 Height;
  uint16 Planes;
  uint16 BitsPerPixel;
  uint32 Compression;
  uint32 SizeOfBitmap;
  uint32  HorzResolution;
  uint32  VertResolution;
  uint32 ColorsUsed;
  uint32 ColorsImportant;
  uint32 RedMask;
  uint32 GreenMask;
  uint32 BlueMask;
  uint32 AlphaMask;
};
#pragma pack(pop)

inline uint8
ProcessPixelWithMask(uint32 Pixel, uint32 Mask) {
  bit_scan_result Result = FindLeastSignificantSetBit(Mask);
  Assert(Result.Found);
  return (uint8)(Pixel >> Result.Index);
}

// this is not a compelete BMP loading procedure
// we just need to load sutff from ourself's
// pixels from bottom to top
internal loaded_bitmap
LoadBMP(thread_context *Thread, debug_platform_read_file ReadFile, char *FileName) {
  loaded_bitmap Result = {};

  debug_read_file_result ReadResult = ReadFile(Thread, FileName);
  if(ReadResult.Memory) {
    bitmap_header *Header = (bitmap_header *)ReadResult.Memory;
    Result.Width = Header->Width;
    Result.Height = Header->Height;
    Assert(Header->Compression == 3);

    Result.Pixel = (uint32 *)((uint8 *)ReadResult.Memory + Header->DataOffset);

    // we have to restructure pixels
    // expected byte order: BB GG RR AA, 0xAARRGGBB
    uint32 *Pixel = Result.Pixel;
    for(int Y = 0; Y < Header->Height; Y++) {
      for(int X = 0; X < Header->Width; X++) {
#if 0
        uint8 A = ProcessPixelWithMask(*Pixel, Header->AlphaMask);
        uint8 R = ProcessPixelWithMask(*Pixel, Header->RedMask);
        uint8 G = ProcessPixelWithMask(*Pixel, Header->GreenMask);
        uint8 B = ProcessPixelWithMask(*Pixel, Header->BlueMask);
        *Pixel++ = (A << 24 | R << 16 | G << 8 | B);
#else
        bit_scan_result RedScan = FindLeastSignificantSetBit(Header->RedMask);
        bit_scan_result GreenScan = FindLeastSignificantSetBit(Header->GreenMask);
        bit_scan_result BlueScan = FindLeastSignificantSetBit(Header->BlueMask);
        bit_scan_result AlphaScan = FindLeastSignificantSetBit(Header->AlphaMask);

        Assert(RedScan.Found);
        Assert(GreenScan.Found);
        Assert(BlueScan.Found);
        Assert(AlphaScan.Found);

        int32 RedShift = 16 - (int32)RedScan.Index;
        int32 GreenShift = 8 - (int32)GreenScan.Index;
        int32 BlueShift = 0 - (int32)BlueScan.Index;
        int32 AlphaShift = 24 - (int32)AlphaScan.Index;

        uint32 C = *Pixel;
        *Pixel++ = RotateLeft(C & Header->RedMask, RedShift) |
          RotateLeft(C & Header->GreenMask, GreenShift) |
          RotateLeft(C & Header->BlueMask, BlueShift) |
          RotateLeft(C & Header->AlphaMask, AlphaShift);
#endif
      }
    }
  }

  return Result;
}

internal void
InitializeArena(memory_arena *Arena, size_t Size, uint8 *Base) {
  Arena->Size = Size;
  Arena->Base = Base;
  Arena->Used = 0;
}

internal void
DrawBitmap(
  game_offscreen_buffer *Buffer,
  loaded_bitmap Bitmap,
  real32 RealX,
  real32 RealY,
  real32 OffsetX = 0,
  real32 OffsetY = 0
  ) {
  int32 MinX = RoundReal32ToInt32(RealX - OffsetX);
  int32 MinY = RoundReal32ToInt32(RealY - OffsetY);
  int32 MaxX = MinX + Bitmap.Width;
  int32 MaxY = MinY + Bitmap.Height;


  int32 ClipX = 0;
  if(MinX < 0) {
    ClipX = -MinX;
    MinX = 0;
  }
  int32 ClipY = 0;
  if(MinY < 0) {
    ClipY = -MinY;
    MinY = 0;
  }
  if(MaxX > Buffer->Width) {
    MaxX = Buffer->Width;
  }
  if(MaxY > Buffer->Height) {
    MaxY = Buffer->Height;
  }

  uint32 *SourceRow = Bitmap.Pixel + ClipY*Bitmap.Width + ClipX;
  uint32 *DestRow = (uint32*)Buffer->Memory + MinY*Buffer->Width + MinX;

  for(int Y = MinY; Y < MaxY; Y++) {
    uint32 *Source = SourceRow;
    uint32 *Dest = DestRow;

    for(int X = MinX; X < MaxX; X++) {
      real32 SA = (real32)((*Source >> 24) & 0xff);
      real32 SR = (real32)((*Source >> 16) & 0xff);
      real32 SG = (real32)((*Source >> 8) & 0xff);
      real32 SB = (real32)((*Source >> 0) & 0xff);

      real32 DR = (real32)((*Dest >> 16) & 0xff);
      real32 DG = (real32)((*Dest >> 8) & 0xff);
      real32 DB = (real32)((*Dest >> 0) &0xff);

      real32 Ratio = SA / 255.0f;
      real32 R = (1 - Ratio)*DR + Ratio*SR;
      real32 G = (1 - Ratio)*DG + Ratio*SG;
      real32 B = (1 - Ratio)*DB + Ratio*SB;
      *Dest = ((uint32)(R + 0.5f) << 16) | ((uint32)(G + 0.5f) << 8) | ((uint32)(B + 0.5f));

      Dest++;
      Source++;
    }

    SourceRow += Bitmap.Width;
    DestRow += Buffer->Width;
  }
}

// exclusive
internal void
DrawRectangle(
  game_offscreen_buffer *Buffer,
  v2 Min,
  v2 Max,
  real32 R, real32 G, real32 B
  ) {
  int32 MinX = RoundReal32ToInt32(Min.X);
  int32 MinY = RoundReal32ToInt32(Min.Y);
  int32 MaxX = RoundReal32ToInt32(Max.X);
  int32 MaxY = RoundReal32ToInt32(Max.Y);

  if(MinX < 0) {
    MinX = 0;
  }
  if(MinY < 0) {
    MinY = 0;
  }
  if(MaxX > Buffer->Width) {
    MaxX = Buffer->Width;
  }
  if(MaxY > Buffer->Height) {
    MaxY = Buffer->Height;
  }

  uint32 Color = (RoundReal32ToUint32(R * 255.0f) << 16) |
  (RoundReal32ToUint32(G * 255.0f) << 8) |
  RoundReal32ToUint32(B * 255.0f);

  uint8 *Row =  (uint8 *)Buffer->Memory + MinY * Buffer->Pitch + MinX * Buffer->BytesPerPixel;

  for(int Y = MinY; Y < MaxY; Y++) {
    uint32 *Pixel = (uint32 *)Row;
    for(int X = MinX; X < MaxX; X++) {
      *Pixel++ = Color;
    }

    Row += Buffer->Pitch;
  }
}

internal void
SetCamera(game_state *State, world_position NewCameraP) {
  v2 EntityOffset = {};
  world *World = &State->World;
  world_diff Delta = SubtractPosition(World, NewCameraP, State->CameraP);
  State->CameraP = NewCameraP;

  EntityOffset = -Delta.dXY;

  int32 TileSpanX = 17*3;
  int32 TileSpanY = 9*3;
  rectangle2 HighFrequencyBound = RectCenterDim(v2{0, 0}, World->TileSizeInMeters * v2{(real32)TileSpanX, (real32)TileSpanY});

  ProcessEntitiesOutsideOfCamera(State, EntityOffset, HighFrequencyBound);

  int32 MinTileX = NewCameraP.AbsTileX - TileSpanX/2;
  int32 MaxTileX = NewCameraP.AbsTileX + TileSpanX/2;

  int32 MinTileY = NewCameraP.AbsTileY - TileSpanY/2;
  int32 MaxTileY = NewCameraP.AbsTileY + TileSpanY/2;

  for(uint32 EntityIndex = 0;
    EntityIndex < State->LowEntityCount;
    EntityIndex++
  ) {
    low_entity *Low = State->LowEntities + EntityIndex;
    if(Low->P.AbsTileZ == NewCameraP.AbsTileZ) {
      if(
        Low->P.AbsTileX >= MinTileX &&
        Low->P.AbsTileX <= MaxTileX &&
        Low->P.AbsTileY <= MaxTileY &&
        Low->P.AbsTileY <= MaxTileY
        ) {
        MakeEntityHighFrequency(State, Low);
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
    real32 Y = RelY + tResult*PlayerDeltaY;
    if(Y >= -RadiusY && Y <= RadiusY) {
      if(tResult > 0.0f && tResult < *tMin) {
        Hit = true;
        *tMin = Maximum(0.0f, tResult - tEpsilon);
      }
    }
  }
  return Hit;
}

internal void
MovePlayer(game_state *State, high_entity *Entity, real32 dt, v2 ddP) {
  world *World = &State->World;
  real32 ddPLength = Length(ddP);
  if(ddPLength > 1) {
    ddP *= (1.0f / ddPLength);
  }

  real32 PlayerSpeed = 80.0f; // m/s^2
  ddP *= PlayerSpeed;
  ddP += -8*Entity->dP;

  v2 PlayerDelta = 0.5f*ddP*Square(dt) + Entity->dP*dt;
  v2 OldPlayerP = Entity->P;
  Entity->dP += ddP*dt;

  for(
    int32 Iteration = 0;
    Iteration < 4;
    Iteration++
  ) {
    real32 tMin = 1.0f;
    high_entity *HitEntity = 0;
    v2 WallNormal = {};
    v2 TargetPlayerP = OldPlayerP + PlayerDelta;

    for(uint32 EntityIndex = 0; EntityIndex < State->HighEntityCount; EntityIndex++) {
      high_entity* TestEntity = State->HighEntities + EntityIndex;
      if(TestEntity != Entity) {
        if(TestEntity->LowEntity->Collides) {
          v2 Rel = Entity->P - TestEntity->P;

          real32 RadiusW = 0.5f*TestEntity->LowEntity->Width + 0.5f*Entity->LowEntity->Width;
          real32 RadiusH = 0.5f*TestEntity->LowEntity->Height + 0.5f*Entity->LowEntity->Height;

          // left
          if(TestWall(-RadiusW, PlayerDelta.X, PlayerDelta.Y, Rel.X, Rel.Y, RadiusH, &tMin)) {
            WallNormal = {1, 0};
            HitEntity = TestEntity;
          }

          // right
          if(TestWall(RadiusW, PlayerDelta.X, PlayerDelta.Y, Rel.X, Rel.Y, RadiusH, &tMin)) {
            WallNormal = {-1, 0};
            HitEntity = TestEntity;
          }

          // bottom
          if(TestWall(-RadiusH, PlayerDelta.Y, PlayerDelta.X, Rel.Y, Rel.X, RadiusW, &tMin)) {
            WallNormal = {0, 1};
            HitEntity = TestEntity;
          }

          // top
          if(TestWall(RadiusH, PlayerDelta.Y, PlayerDelta.X, Rel.Y, Rel.X, RadiusW, &tMin)) {
            WallNormal = {0, -1};
            HitEntity = TestEntity;
          }
        }
      }
    }

    Entity->P += tMin*PlayerDelta;
    if(HitEntity) {
      Entity->dP = Entity->dP - 1*Inner(Entity->dP, WallNormal)*WallNormal;
      PlayerDelta = TargetPlayerP - Entity->P;
      PlayerDelta = PlayerDelta - 1*Inner(PlayerDelta, WallNormal)*WallNormal;
      Entity->AbsTileZ += HitEntity->LowEntity->dAbsTileZ;
    } else {
      break;
    }
  }

  if(AbsoluteValue(Entity->dP.X) > AbsoluteValue(Entity->dP.Y)) {
    if(Entity->dP.X > 0) {
      Entity->FacingDirection = 0;
    } else {
      Entity->FacingDirection = 2;
    }
  } else {
    if(Entity->dP.Y > 0) {
      Entity->FacingDirection = 1;
    } else {
      Entity->FacingDirection = 3;
    }
  }

  Entity->LowEntity->P = Offset(World, State->CameraP, Entity->P);
}

extern "C" GAME_UPDATE_VIDEO(GameUpdateVideo) {
  Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == ArrayCount(Input->Controllers[0].Buttons))

  Assert(sizeof(game_state) <= Memory->PermanentStorageSize)
  game_state *State = (game_state *)(Memory->PermanentStorage);
  memory_arena *MemoryArena = &State->MemoryArena;
  world *World = &State->World;

  real32 TileSizeInPixels = 60.0f;
  World->TileSizeInMeters = 1.4f;
  real32 MetersToPixels = TileSizeInPixels / World->TileSizeInMeters;

  uint32 TilesPerWidth = 17;
  uint32 TilesPerHeight = 9;

  if(!Memory->IsInitialized) {
    Memory->IsInitialized = true;
    State->Background = LoadBMP(Thread, Memory->DebugPlatformReadFile, "test/test_background.bmp");

    hero_bitmaps *HeroBitmaps = &State->HeroBitmaps[0];
    HeroBitmaps->OffsetX = 72;
    HeroBitmaps->OffsetY = 35;
    HeroBitmaps->Head = LoadBMP(Thread, Memory->DebugPlatformReadFile, "test/test_hero_right_head.bmp");
    HeroBitmaps->Cape = LoadBMP(Thread, Memory->DebugPlatformReadFile, "test/test_hero_right_cape.bmp");
    HeroBitmaps->Torso = LoadBMP(Thread, Memory->DebugPlatformReadFile, "test/test_hero_right_torso.bmp");
    HeroBitmaps++;

    HeroBitmaps->OffsetX = 72;
    HeroBitmaps->OffsetY = 35;
    HeroBitmaps->Head = LoadBMP(Thread, Memory->DebugPlatformReadFile, "test/test_hero_back_head.bmp");
    HeroBitmaps->Cape = LoadBMP(Thread, Memory->DebugPlatformReadFile, "test/test_hero_back_cape.bmp");
    HeroBitmaps->Torso = LoadBMP(Thread, Memory->DebugPlatformReadFile, "test/test_hero_back_torso.bmp");
    HeroBitmaps++;

    HeroBitmaps->OffsetX = 72;
    HeroBitmaps->OffsetY = 35;
    HeroBitmaps->Head = LoadBMP(Thread, Memory->DebugPlatformReadFile, "test/test_hero_left_head.bmp");
    HeroBitmaps->Cape = LoadBMP(Thread, Memory->DebugPlatformReadFile, "test/test_hero_left_cape.bmp");
    HeroBitmaps->Torso = LoadBMP(Thread, Memory->DebugPlatformReadFile, "test/test_hero_left_torso.bmp");
    HeroBitmaps++;

    HeroBitmaps->OffsetX = 72;
    HeroBitmaps->OffsetY = 35;
    HeroBitmaps->Head = LoadBMP(Thread, Memory->DebugPlatformReadFile, "test/test_hero_front_head.bmp");
    HeroBitmaps->Cape = LoadBMP(Thread, Memory->DebugPlatformReadFile, "test/test_hero_front_cape.bmp");
    HeroBitmaps->Torso = LoadBMP(Thread, Memory->DebugPlatformReadFile, "test/test_hero_front_torso.bmp");

    InitializeArena(MemoryArena, Memory->PermanentStorageSize - sizeof(game_state), (uint8 *)Memory->PermanentStorage + sizeof(game_state));

    world_position NewCameraP = {};
    NewCameraP.AbsTileX = 8;
    NewCameraP.AbsTileY = 4;
    NewCameraP.AbsTileZ = 0;

    World->ChunkShift = 4;
    World->ChunkDim = 1 << World->ChunkShift;
    World->ChunkMask = World->ChunkDim - 1;

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
    uint32 AbsTileZ = 0;

    for(uint32 ScreenIndex = 0; ScreenIndex < 10; ScreenIndex++) {
      Assert(RandomIndex < ArrayCount(RandomNumberTable));
      uint32 RandomValue;

      // 0: left
      // 1: bottom
      // 2: up/down
      if(1) { // (DoorUp || DoorDown) {
        RandomValue = RandomNumberTable[RandomIndex++] % 2;
      } else {
        RandomValue = RandomNumberTable[RandomIndex++] % 3;
      }

      switch(RandomValue) {
        case 0: {
          DoorLeft = true;
        } break;

        case 1: {
          DoorBottom = true;
        } break;

        case 2:
          if(AbsTileZ == 0) {
            DoorUp = true;
          } else if(AbsTileZ == 1) {
            DoorDown = true;
          }
          break;
      }

      for(uint32 TileY = 0; TileY < TilesPerHeight; TileY++) {
        for(uint32 TileX = 0; TileX < TilesPerWidth; TileX++) {
          uint32 AbsTileX = ScreenX*TilesPerWidth + TileX;
          uint32 AbsTileY = ScreenY*TilesPerHeight + TileY;
          uint32 TileValue = 1;

          if(TileX == 0) {
            TileValue = 2;
            if(DoorLeft && TileY == (TilesPerHeight / 2)) {
              TileValue = 1;
            }
          }
          if(TileX == (TilesPerWidth - 1)) {
            TileValue = 2;
            if(DoorRight && TileY == (TilesPerHeight / 2)) {
              TileValue = 1;
            }
          }
          if(TileY == 0) {
            TileValue = 2;
            if(DoorBottom && TileX == (TilesPerWidth / 2)) {
              TileValue = 1;
            }
          }
          if(TileY == (TilesPerHeight - 1)) {
            TileValue = 2;
            if(DoorTop && TileX == (TilesPerWidth / 2)) {
              TileValue = 1;
            }
          }

          if(TileX == 10 && TileY == 4 && (DoorUp || DoorDown)) {
            TileValue = 3;
          }

          if(TileValue == 2) {
            AddWall(State, AbsTileX, AbsTileY, AbsTileZ);
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
        if(AbsTileZ == 0) {
          DoorDown = true;
          DoorUp = false;
          AbsTileZ = 1;
        } else {
          DoorUp = true;
          DoorDown = false;
          AbsTileZ = 0;
        }
      } else {
        DoorDown = false;
        DoorUp = false;
      }
    }

    SetCamera(State, NewCameraP);
  }

#define TILE_COUNT_X 256
#define TILE_COUNT_Y 256
#define tile_chunk_COUNT_X 2
#define tile_chunk_COUNT_Y 2
  v2 ScreenCenter = {(real32)Buffer->Width / 2, (real32)Buffer->Height / 2};

  for(
    int ControllerIndex = 0;
    ControllerIndex < ArrayCount(Input->Controllers);
    ControllerIndex++
  ) {
    game_controller_input *Controller = &Input->Controllers[ControllerIndex];
    low_entity *LowEntity =  State->PlayerEntityForController[ControllerIndex];
    if(LowEntity) {
      v2 ddP = {};

      if(Controller->IsAnalog) {
        ddP = {Controller->StickAverageX, Controller->StickAverageY};
      } else {
        if(Controller->MoveUp.IsEndedDown) {
          ddP.Y = 1.0f;
        }
        if(Controller->MoveDown.IsEndedDown) {
          ddP.Y = -1.0f;
        }
        if(Controller->MoveLeft.IsEndedDown) {
          ddP.X = -1.0f;
        }
        if(Controller->MoveRight.IsEndedDown) {
          ddP.X = 1.0f;
        }
      }

      MovePlayer(State, LowEntity->HighEntity, Input->dt, ddP);
    } else {
      if(Controller->Start.IsEndedDown) {
        high_entity *PlayerEntity = AddPlayer(State);
        State->PlayerEntityForController[ControllerIndex] = PlayerEntity->LowEntity;

        if(!State->CameraFollowingEntity) {
          State->CameraFollowingEntity = PlayerEntity->LowEntity;
        }
      }
    }
  }

  low_entity *CameraFollowingEntity = State->CameraFollowingEntity;
  if(CameraFollowingEntity) {
    bool32 HasChanged = false;
    world_position NewCameraP = State->CameraP;
    if(CameraFollowingEntity->HighEntity->AbsTileZ != NewCameraP.AbsTileZ) {
      NewCameraP.AbsTileZ = CameraFollowingEntity->HighEntity->AbsTileZ;
      HasChanged = true;
    }
    v2 Diff = CameraFollowingEntity->HighEntity->P;

    if(Diff.X > ((real32)TilesPerWidth / 2)*World->TileSizeInMeters) {
      HasChanged = true;
      NewCameraP.AbsTileX += TilesPerWidth;
    }
    if(Diff.X < -((real32)TilesPerWidth / 2)*World->TileSizeInMeters) {
      HasChanged = true;
      NewCameraP.AbsTileX -= TilesPerWidth;
    }
    if(Diff.Y > ((real32)TilesPerHeight / 2)*World->TileSizeInMeters) {
      HasChanged = true;
      NewCameraP.AbsTileY += TilesPerHeight;
    }
    if(Diff.Y < -((real32)TilesPerHeight / 2)*World->TileSizeInMeters) {
      HasChanged = true;
      NewCameraP.AbsTileY -= TilesPerHeight;
    }

    if(HasChanged) {
      SetCamera(State, NewCameraP);
    }
  }

  DrawBitmap(Buffer, State->Background, 0, 0);

#if 0
  for(int RelY = -4; RelY <= 4; RelY++) {
    for(int RelX = -8; RelX <= 8; RelX++) {
      real32 Gray = 1.0f; // block

      v2 Rel = {(real32)RelX * TileSizeInPixels, (real32)RelY * TileSizeInPixels};
      v2 TileCenter = ScreenCenter - State->CameraP.Offset_*MetersToPixels + Rel;
      v2 HalfTileSize = {0.5f*TileSizeInPixels, 0.5f*TileSizeInPixels};
      v2 TileBottomLeft = TileCenter - HalfTileSize;
      v2 TileTopRight = TileCenter + HalfTileSize;

      uint32 AbsX = RelX + State->CameraP.AbsTileX;
      uint32 AbsY = RelY + State->CameraP.AbsTileY;

      uint32 TileValue = GetTileValue(TileMap, AbsX, AbsY, State->CameraP.AbsTileZ);

      if(TileValue > 1) {
        if(TileValue == 1) {
          Gray = 0.5f;
        }

        if(TileValue == 3) {
          Gray = 0.25f;
        }

        if(AbsX == State->CameraP.AbsTileX &&
          AbsY == State->CameraP.AbsTileY) {
          Gray = 0.0f;
        }

        DrawRectangle(Buffer, TileBottomLeft, TileTopRight, Gray, Gray, Gray);
      }
    }
  }
#endif

  for(
    uint32 EntityIndex = 0;
    EntityIndex < State->HighEntityCount;
    EntityIndex++
  ) {
    high_entity *Entity = State->HighEntities + EntityIndex;
    v2 EntityGround = ScreenCenter + Entity->P*MetersToPixels;

    v2 EntityMin = EntityGround + v2{-0.5f*Entity->LowEntity->Width*MetersToPixels, -0.5f*Entity->LowEntity->Height*MetersToPixels};
    v2 EntityMax = EntityGround + v2{0.5f*Entity->LowEntity->Width*MetersToPixels, 0.5f*Entity->LowEntity->Height*MetersToPixels};

    if(Entity->LowEntity->Type == EntityType_Hero) {
      DrawRectangle(Buffer, EntityMin, EntityMax, 1.0f, 1.0f, 0.0f);
      hero_bitmaps HeroBitmaps = State->HeroBitmaps[Entity->FacingDirection];
      DrawBitmap(Buffer, HeroBitmaps.Torso, EntityGround.X, EntityGround.Y, HeroBitmaps.OffsetX, HeroBitmaps.OffsetY);
      DrawBitmap(Buffer, HeroBitmaps.Cape, EntityGround.X, EntityGround.Y, HeroBitmaps.OffsetX, HeroBitmaps.OffsetY);
      DrawBitmap(Buffer, HeroBitmaps.Head, EntityGround.X, EntityGround.Y, HeroBitmaps.OffsetX, HeroBitmaps.OffsetY);
    } else {
      DrawRectangle(Buffer, EntityMin, EntityMax, 1.0f, 1.0f, 0.0f);
    }
  }
}

extern "C" GAME_UPDATE_AUDIO(GameUpdateAudio) {
}

