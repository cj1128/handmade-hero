#include "handmade.h"
#include "handmade_random.h"

internal void
ChangeEntityResidence(game_state *State, uint32 EntityIndex, entity_residence Residence) {
  State->EntityResidence[EntityIndex] = Residence;
}

internal uint32
AddEntity(game_state *State) {
  uint32 Index = State->EntityCount++;
  Assert(Index < ArrayCount(State->HighEntities));
  return Index;
}

internal void
InitializePlayer(entity Entity) {
  Entity.High->FacingDirection = 0;
  Entity.Dormant->P.AbsTileX = 2;
  Entity.Dormant->P.AbsTileY = 3;
  Entity.Dormant->P.AbsTileZ = 0;
  Entity.Dormant->P.Offset_ = {};
  Entity.Dormant->Height = 0.5f;
  Entity.Dormant->Width = 1.0f;
  Entity.Dormant->Collides = true;
}

internal entity
GetEntity(game_state *State, uint32 Index) {
  entity Result = {};

  if(Index > 0 && Index < ArrayCount(State->HighEntities)) {
    Result.Residence = State->EntityResidence[Index];
    Result.High = &State->HighEntities[Index];
    Result.Low = &State->LowEntities[Index];
    Result.Dormant = &State->DormantEntities[Index];
  }

  return Result;
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
RenderWeirdGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset) {
  uint8 *Row = (uint8 *)Buffer->Memory;

  for(int Y = 0; Y < Buffer->Height; Y++) {
    uint32 *Pixel = (uint32 *)Row;

    for(int X = 0; X < Buffer->Width; X++) {
      uint8 Blue = (uint8)(X + XOffset);
      uint8 Green = (uint8)(Y + YOffset);
      // 0xXXRRGGBB
      *Pixel++ = (Green << 16) | Blue;
    }

    Row = Row + Buffer->Pitch;
  }
}

internal void
RenderSineWave(
  game_state *State,
  game_sound_buffer *Buffer
  ) {
#if 0
  int ToneHz = State->ToneHz;
  int WavePeriod = Buffer->SamplesPerSecond / ToneHz;

  int16* Output = Buffer->Memory;
  for(int SampleIndex = 0; SampleIndex < Buffer->SampleCount; SampleIndex++) {
    int16 Value = (int16)(sinf(State->tSine) * Buffer->ToneVolume);
    *Output++ = Value;
    *Output++ = Value;
    State->tSine += (real32)(2.0f * Pi32 * (1.0f / (real32)WavePeriod));
    if(State->tSine > 2.0f * Pi32) {
      State->tSine -= (real32)(2.0f * Pi32);
    }
  }
#endif
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
MovePlayer(game_state *State, entity Entity, real32 dt, v2 ddP) {
  tile_map *TileMap = &State->World.TileMap;
  real32 ddPLength = Length(ddP);
  if(ddPLength > 1) {
    ddP *= (1.0f / ddPLength);
  }

  real32 PlayerSpeed = 80.0f; // m/s^2
  ddP *= PlayerSpeed;
  ddP += -8*Entity.High->dP;

  v2 PlayerDelta = 0.5f*ddP*Square(dt) + Entity.High->dP*dt;
  v2 OldPlayerP = Entity.High->P;
  v2 NewPlayerP = OldPlayerP + PlayerDelta;
  Entity.High->dP += ddP*dt;

  real32 tRemaining = 1.0f;

  for(
    int32 Iteration = 0;
    Iteration < 4 && tRemaining > 0;
    Iteration++
  ) {
    real32 tMin = 1.0f;
    uint32 HitEntityIndex = 0;
    v2 WallNormal = {};

    for(uint32 EntityIndex = 1; EntityIndex < State->EntityCount; EntityIndex++) {
      entity TestEntity = GetEntity(State, EntityIndex);
      if(TestEntity.Residence == EntityResidence_High && TestEntity.High != Entity.High) {
        if(TestEntity.Dormant->Collides) {
          v2 Rel = Entity.High->P - TestEntity.High->P;

          real32 RadiusW = 0.5f*TestEntity.Dormant->Width + 0.5f*Entity.Dormant->Width;
          real32 RadiusH = 0.5f*TestEntity.Dormant->Height + 0.5f*Entity.Dormant->Height;

          // left
          if(TestWall(-RadiusW, PlayerDelta.X, PlayerDelta.Y, Rel.X, Rel.Y, RadiusH, &tMin)) {
            WallNormal = {1, 0};
            HitEntityIndex = EntityIndex;
          }
            // right
          if(TestWall(RadiusW, PlayerDelta.X, PlayerDelta.Y, Rel.X, Rel.Y, RadiusH, &tMin)) {
            WallNormal = {-1, 0};
            HitEntityIndex = EntityIndex;
          }
            // bottom
          if(TestWall(-RadiusH, PlayerDelta.Y, PlayerDelta.X, Rel.Y, Rel.X, RadiusW, &tMin)) {
            WallNormal = {0, 1};
            HitEntityIndex = EntityIndex;
          }
            // top
          if(TestWall(RadiusH, PlayerDelta.Y, PlayerDelta.X, Rel.Y, Rel.X, RadiusW, &tMin)) {
            WallNormal = {0, -1};
            HitEntityIndex = EntityIndex;
          }
        }
      }
    }

    Entity.High->P += tMin*PlayerDelta;
    if(HitEntityIndex) {
      Entity.High->dP = Entity.High->dP - 1*Inner(Entity.High->dP, WallNormal)*WallNormal;
      PlayerDelta = PlayerDelta - 1*Inner(PlayerDelta, WallNormal)*WallNormal;
      PlayerDelta *= (1 - tMin);
      tRemaining -= tMin*tRemaining;

      entity HitEntity = GetEntity(State, HitEntityIndex);
      Entity.High->AbsTileZ += HitEntity.Dormant->dAbsTileZ;
    } else {
      break;
    }
  }

  if(AbsoluteValue(Entity.High->dP.X) > AbsoluteValue(Entity.High->dP.Y)) {
    if(Entity.High->dP.X > 0) {
      Entity.High->FacingDirection = 0;
    } else {
      Entity.High->FacingDirection = 2;
    }
  } else {
    if(Entity.High->dP.Y > 0) {
      Entity.High->FacingDirection = 1;
    } else {
      Entity.High->FacingDirection = 3;
    }
  }

  Entity.Dormant->P = Offset(TileMap, State->CameraP, Entity.High->P);
}

extern "C" GAME_UPDATE_VIDEO(GameUpdateVideo) {
  Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == ArrayCount(Input->Controllers[0].Buttons))

  Assert(sizeof(game_state) <= Memory->PermanentStorageSize)
  game_state *State = (game_state *)(Memory->PermanentStorage);
  memory_arena *MemoryArena = &State->MemoryArena;
  world *World = &State->World;
  tile_map *TileMap = &World->TileMap;

  real32 TileSizeInPixels = 60.0f;
  TileMap->TileSizeInMeters = 1.4f;
  real32 MetersToPixels = TileSizeInPixels / TileMap->TileSizeInMeters;

  uint32 TilesPerWidth = 17;
  uint32 TilesPerHeight = 9;

  if(!Memory->IsInitialized) {
    Memory->IsInitialized = true;
    AddEntity(State); // reserve zero

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

    State->CameraP = {};
    State->CameraP.AbsTileX = 8;
    State->CameraP.AbsTileY = 4;
    State->CameraP.AbsTileZ = 0;

    TileMap->TileChunkShift = 4;
    TileMap->TileChunkDim = 1 << TileMap->TileChunkShift;
    TileMap->TileChunkMask = TileMap->TileChunkDim - 1;

    TileMap->TileChunkCountX = 128;
    TileMap->TileChunkCountY = 128;
    TileMap->TileChunkCountZ = 2;

    uint32 TileChunkCount = TileMap->TileChunkCountZ * TileMap->TileChunkCountX * TileMap->TileChunkCountY;

    TileMap->TileChunks = PushArray(MemoryArena, TileChunkCount, tile_chunk);

    uint32 ScreenX = 0;
    uint32 ScreenY = 0;
    uint32 RandomIndex = 0;
    bool32 DoorLeft = false;
    bool32 DoorRight = false;
    bool32 DoorTop = false;
    bool32 DoorBottom = false;
    bool32 DoorUp = false;
    bool32 DoorDown = false;
    uint32 AbsTileZ = 0;

    for(uint32 ScreenIndex = 0; ScreenIndex < 100; ScreenIndex++) {
      Assert(RandomIndex < ArrayCount(RandomNumberTable));
      uint32 RandomValue;

      if(DoorUp || DoorDown) {
        RandomValue = RandomNumberTable[RandomIndex++] % 2;
      } else {
        RandomValue = RandomNumberTable[RandomIndex++] % 3;
      }

      if(RandomValue == 0) {
        DoorRight = true;
      } else if(RandomValue == 1) {
        DoorTop = true;
      } else if(RandomValue == 2) {
        if(AbsTileZ == 0) {
          DoorUp = true;
        } else if(AbsTileZ == 1) {
          DoorDown = true;
        }
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

          SetTileValue(MemoryArena, TileMap, AbsTileX, AbsTileY, AbsTileZ, TileValue);
        }
      }

      DoorLeft = DoorRight;
      DoorBottom = DoorTop;
      DoorRight = false;
      DoorTop = false;

      if(RandomValue == 0) {
        ScreenX++;
        DoorUp = false;
        DoorDown = false;
      } else if(RandomValue == 1) {
        ScreenY++;
        DoorUp = false;
        DoorDown = false;
      } else if(RandomValue == 2) {
        if(DoorUp) {
          DoorDown = true;
          DoorUp = false;
        } else if(DoorDown) {
          DoorUp = true;
          DoorDown = false;
        }

        if(AbsTileZ == 0) {
          AbsTileZ = 1;
        } else {
          AbsTileZ = 0;
        }
      }
    }
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
    entity Entity = GetEntity(State, State->PlayerIndexForController[ControllerIndex]);
    if(Entity.Residence == EntityResidence_High) {
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

      MovePlayer(State, Entity, Input->dt, ddP);
    } else {
      if(Controller->Start.IsEndedDown) {
        uint32 EntityIndex = AddEntity(State);
        InitializePlayer(GetEntity(State, EntityIndex));
        ChangeEntityResidence(State, EntityIndex, EntityResidence_High);

        State->PlayerIndexForController[ControllerIndex] = EntityIndex;

        if(!State->CameraFollowingEntityIndex) {
          State->CameraFollowingEntityIndex = EntityIndex;
        }
      }
    }
  }

  v2 EntityOffset = {};

  entity CameraFollowingEntity = GetEntity(State, State->CameraFollowingEntityIndex);
  if(CameraFollowingEntity.Residence == EntityResidence_High) {
    State->CameraP.AbsTileZ = CameraFollowingEntity.High->AbsTileZ;
    v2 Diff = CameraFollowingEntity.High->P;
    tile_map_position OldCameraP = State->CameraP;

    if(Diff.X > ((real32)TilesPerWidth / 2)*TileMap->TileSizeInMeters) {
      State->CameraP.AbsTileX += TilesPerWidth;
    }
    if(Diff.X < -((real32)TilesPerWidth / 2)*TileMap->TileSizeInMeters) {
      State->CameraP.AbsTileX -= TilesPerWidth;
    }
    if(Diff.Y > ((real32)TilesPerHeight / 2)*TileMap->TileSizeInMeters) {
      State->CameraP.AbsTileY += TilesPerHeight;
    }
    if(Diff.Y < -((real32)TilesPerHeight / 2)*TileMap->TileSizeInMeters) {
      State->CameraP.AbsTileY -= TilesPerHeight;
    }

    tile_map_diff Delta = SubtractPosition(TileMap, State->CameraP, OldCameraP);
    EntityOffset = -Delta.dXY;
  }

  DrawBitmap(Buffer, State->Background, 0, 0);

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

  for(
    uint32 EntityIndex = 0;
    EntityIndex < State->EntityCount;
    EntityIndex++
  ) {
    entity Entity = GetEntity(State, EntityIndex);
    if(Entity.Residence == EntityResidence_High) {
      Entity.High->P += EntityOffset;
      v2 PlayerGround = ScreenCenter + Entity.High->P*MetersToPixels;

      v2 PlayerMin = PlayerGround + v2{-0.5f*Entity.Dormant->Width*MetersToPixels, -0.5f*Entity.Dormant->Height*MetersToPixels};
      v2 PlayerMax = PlayerGround + v2{0.5f*Entity.Dormant->Width*MetersToPixels, 0.5f*Entity.Dormant->Height*MetersToPixels};

      DrawRectangle(Buffer, PlayerMin, PlayerMax, 1.0f, 1.0f, 0.0f);

      hero_bitmaps HeroBitmaps = State->HeroBitmaps[Entity.High->FacingDirection];
      DrawBitmap(Buffer, HeroBitmaps.Torso, PlayerGround.X, PlayerGround.Y, HeroBitmaps.OffsetX, HeroBitmaps.OffsetY);
      DrawBitmap(Buffer, HeroBitmaps.Cape, PlayerGround.X, PlayerGround.Y, HeroBitmaps.OffsetX, HeroBitmaps.OffsetY);
      DrawBitmap(Buffer, HeroBitmaps.Head, PlayerGround.X, PlayerGround.Y, HeroBitmaps.OffsetX, HeroBitmaps.OffsetY);
    }
  }
}

extern "C" GAME_UPDATE_AUDIO(GameUpdateAudio) {
}

