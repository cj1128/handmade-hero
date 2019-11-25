#include "handmade.h"

internal void
InitializeArena(memory_arena *Arena, size_t Size, uint8 *Base) {
  Arena->Size = Size;
  Arena->Base = Base;
  Arena->Used = 0;
}

// exclusive
internal void
DrawRectangle(game_offscreen_buffer *Buffer, real32 RealMinX, real32 RealMinY, real32 RealMaxX, real32 RealMaxY, real32 R, real32 G, real32 B) {
  int32 MinX = RoundReal32ToInt32(RealMinX);
  int32 MinY = RoundReal32ToInt32(RealMinY);
  int32 MaxX = RoundReal32ToInt32(RealMaxX);
  int32 MaxY = RoundReal32ToInt32(RealMaxY);

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

internal void *
PushSize_(memory_arena *Arena, size_t Size) {
  Assert((Arena->Used + Size) <= Arena->Size);
  uint8 *Result = Arena->Base + Arena->Used;
  Arena->Used += Size;
  return (void *)Result;
}

#define PushArray(Arena, Count, Type) (Type *)PushArray_((Arena), (Count), sizeof(Type))

internal void *
PushArray_(memory_arena *Arena, uint32 Count, size_t Size) {
  return PushSize_(Arena, Count*Size);
}

extern "C" GAME_UPDATE_VIDEO(GameUpdateVideo) {
  Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == ArrayCount(Input->Controllers[0].Buttons))

  Assert(sizeof(game_state) <= Memory->PermanentStorageSize)
  game_state *State = (game_state *)(Memory->PermanentStorage);
  memory_arena *MemoryArena = &State->MemoryArena;
  world *World = &State->World;
  tile_map *TileMap = &World->TileMap;

  if(!Memory->IsInitialized) {
    Memory->IsInitialized = true;

    InitializeArena(&State->MemoryArena, Memory->PermanentStorageSize - sizeof(game_state), (uint8 *)Memory->PermanentStorage + sizeof(game_state));

    State->PlayerPos = {};
    State->PlayerPos.AbsTileX = 1;
    State->PlayerPos.AbsTileY = 3;
    State->PlayerPos.TileRelX = 0.0f;
    State->PlayerPos.TileRelY = 0.0f;

    TileMap->TileChunkShift = 8;
    TileMap->TileChunkDim = 1 << TileMap->TileChunkShift;
    TileMap->TileChunkMask = TileMap->TileChunkDim - 1;

    TileMap->TileSizeInMeters = 1.4f;
    TileMap->TileSizeInPixels = 55.0f;
    TileMap->MetersToPixels = TileMap->TileSizeInPixels / TileMap->TileSizeInMeters;

    TileMap->TileChunkCountX = 8;
    TileMap->TileChunkCountY = 8;

    uint32 TileChunkCount = TileMap->TileChunkCountX*TileMap->TileChunkCountY;

    TileMap->TileChunks = PushArray(MemoryArena, TileChunkCount, tile_chunk);
    for(uint32 Index = 0; Index < TileChunkCount; Index++) {
      TileMap->TileChunks[Index].Tiles = PushArray(MemoryArena, TileMap->TileChunkDim*TileMap->TileChunkDim, uint32);
    }

    uint32 TilesPerWidth = 17;
    uint32 TilesPerHeight = 9;
    for(uint32 ScreenY = 0; ScreenY < 32; ScreenY++) {
      for(uint32 ScreenX = 0; ScreenX < 32; ScreenX++) {
        for(uint32 TileY = 0; TileY < TilesPerHeight; TileY++) {
          for(uint32 TileX = 0; TileX < TilesPerWidth; TileX++) {
            uint32 AbsTileX = ScreenX*TilesPerWidth + TileX;
            uint32 AbsTileY = ScreenY*TilesPerHeight + TileY;
            SetTileValue(TileMap, AbsTileX, AbsTileY, (TileX == TileY) && (TileX % 2)? 1 : 0);
          }
        }
      }
    }
  }

#define TILE_COUNT_X 256
#define TILE_COUNT_Y 256
#define tile_chunk_COUNT_X 2
#define tile_chunk_COUNT_Y 2
  real32 CenterX = (real32)Buffer->Width / 2;
  real32 CenterY = (real32)Buffer->Height / 2;

  real32 PlayerHeight = 1.4f;
  real32 PlayerWidth = 0.75f * PlayerHeight;

  for(int i = 0; i < ArrayCount(Input->Controllers); i++) {
    game_controller_input *Controller = &Input->Controllers[i];
    if(Controller->IsAnalog) {

    } else {
      real32 dx = 0.0f;
      real32 dy = 0.0f;
      real32 Speed = 2.0f;
      if(Controller->MoveUp.IsEndedDown) {
        dy = 1.0f;
      }
      if(Controller->MoveDown.IsEndedDown) {
        dy = -1.0f;
      }
      if(Controller->MoveLeft.IsEndedDown) {
        dx = -1.0f;
      }
      if(Controller->MoveRight.IsEndedDown) {
        dx = 1.0f;
      }
      if(Controller->ActionUp.IsEndedDown) {
        Speed = 10.0f;
      }

      dx *= Speed;
      dy *= Speed;

      tile_map_position NewPos = State->PlayerPos;

      NewPos.TileRelX += dx * Input->dt;
      NewPos.TileRelY += dy * Input->dt;
      NewPos = RecononicalizePosition(TileMap, NewPos);

      tile_map_position NewPosLeft = NewPos;
      NewPosLeft.TileRelX -= PlayerWidth * 0.5f;
      NewPosLeft = RecononicalizePosition(TileMap, NewPosLeft);

      tile_map_position NewPosRight = NewPos;
      NewPosRight.TileRelX += PlayerWidth * 0.5f;
      NewPosRight = RecononicalizePosition(TileMap, NewPosRight);

      if(IsTileMapEmtpy(TileMap, NewPos) &&
        IsTileMapEmtpy(TileMap, NewPosLeft) &&
        IsTileMapEmtpy(TileMap, NewPosRight)) {
        State->PlayerPos = NewPos;
      }
    }
  }

  // background
  DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height, 0.4f, 0.0f, 0.0f);

  real32 H = (real32)(Buffer->Height);

  for(int RelY = -10; RelY < 10; RelY++) {
    for(int RelX = -10; RelX < 10; RelX++) {
      real32 Gray = 1.0f; // block

      real32 TileCenterX = CenterX - State->PlayerPos.TileRelX*TileMap->MetersToPixels + (real32)RelX*TileMap->TileSizeInPixels;
      real32 TileMinX = TileCenterX - 0.5f*TileMap->TileSizeInPixels;
      real32 TileMaxX = TileMinX + TileMap->TileSizeInPixels;

      real32 TileCenterY = CenterY - State->PlayerPos.TileRelY*TileMap->MetersToPixels + (real32)RelY*TileMap->TileSizeInPixels;
      real32 TileMinY = TileCenterY - 0.5f*TileMap->TileSizeInPixels;
      real32 TileMaxY = TileMinY + TileMap->TileSizeInPixels;

      uint32 AbsX = RelX + State->PlayerPos.AbsTileX;
      uint32 AbsY = RelY + State->PlayerPos.AbsTileY;

      uint32 V = GetTileValue(TileMap, AbsX, AbsY);

      // empty
      if(V == 0) {
        Gray = 0.5f;
      }

      // unknown
      if(V == 2) {
        Gray = 0.2f;
      }

      if(AbsY == State->PlayerPos.AbsTileY && AbsX == State->PlayerPos.AbsTileX) {
        Gray = 0.0f;
      }

      DrawRectangle(Buffer, TileMinX, H - TileMaxY, TileMaxX, H - TileMinY, Gray, Gray, Gray);
    }
  }

  real32 PlayerMinX = CenterX - 0.5f*PlayerWidth*TileMap->MetersToPixels;
  real32 PlayerMaxX = PlayerMinX + PlayerWidth * TileMap->MetersToPixels;
  real32 PlayerMinY = CenterY;
  real32 PlayerMaxY = PlayerMinY + PlayerHeight * TileMap->MetersToPixels;

  DrawRectangle(Buffer, PlayerMinX, H - PlayerMaxY, PlayerMaxX, H - PlayerMinY, 1.0f, 0.0f, 1.0f);
}

extern "C" GAME_UPDATE_AUDIO(GameUpdateAudio) {
}
