#include "handmade.h"

internal int32
RoundReal32ToUint32(real32 Input) {
  uint32 Result = (uint32)(Input + 0.5f);
  return Result;
}

#include "math.h"
inline int32
FloorReal32ToInt32(real32 Value) {
  return (int32)(floorf(Value));
}

// exclusive
internal void
DrawRectangle(game_offscreen_buffer *Buffer, real32 RealMinX, real32 RealMinY, real32 RealMaxX, real32 RealMaxY, real32 R, real32 G, real32 B) {
  int MinX = RoundReal32ToUint32(RealMinX);
  int MinY = RoundReal32ToUint32(RealMinY);
  int MaxX = RoundReal32ToUint32(RealMaxX);
  int MaxY = RoundReal32ToUint32(RealMaxY);

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
IsTileMapEmpty(world *World, tile_map *TileMap, real32 PlayerX, real32 PlayerY) {
  bool32 IsEmtpy = false;
  int TileX = FloorReal32ToInt32((PlayerX - World->UpperLeftX) / World->TileSize);
  int TileY = FloorReal32ToInt32((PlayerY - World->UpperLeftY) / World->TileSize);

  if(TileX >= 0 && TileX < World->TileCountX && TileY >= 0 && TileY < World->TileCountY) {
    uint32 TileValue = TileMap->Tiles[TileY * World->TileCountX + TileX];
    if(TileValue == 0) {
      IsEmtpy = true;
    }
  }

  return IsEmtpy;
}

inline tile_map *
GetTileMap(world *World, int32 X, int32 Y) {
  tile_map *Result = 0;
  if(X >= 0 && X < World->TileMapCountX && Y >= 0 && Y < World->TileMapCountY) {
    Result = &World->TileMaps[Y * World->TileMapCountX + X];
  }

  return Result;
}

inline uint32 GetTileValueUnchecked(world *World, tile_map *TileMap, int32 X, int32 Y) {
  Assert(X >= 0 && X < World->TileCountX);
  Assert(Y >= 0 && Y < World->TileCountY);
  return TileMap->Tiles[Y * World->TileCountX + X];
}

internal canonical_postion
GetCanonicalPosition(world *World, raw_position Pos) {
  int32 TileMapX = Pos.TileMapX;
  int32 TileMapY = Pos.TileMapY;
  real32 X = Pos.X - World->UpperLeftX;
  real32 Y = Pos.Y - World->UpperLeftX;
  int32 TileX = FloorReal32ToInt32(X / World->TileSize);
  int32 TileY = FloorReal32ToInt32(Y / World->TileSize);
  real32 TileRelX = X - TileX * World->TileSize;
  real32 TileRelY = Y - TileY * World->TileSize;

  Assert(TileRelX >= 0 && TileRelX < World->TileSize);
  Assert(TileRelY >= 0 && TileRelY < World->TileSize);

  if(TileX < 0) {
    int32 Delta = (-TileX / World->TileCountX) + 1;
    TileMapX -= Delta;
    TileX = (World->TileCountX + (TileX % World->TileCountX)) % World->TileCountX;
  }
  if(TileX >= World->TileCountX) {
    TileMapX += TileX / World->TileCountX;
    TileX = TileX % World->TileCountX;
  }

  if(TileY < 0) {
    int32 Delta = (-TileY / World->TileCountY) + 1;
    TileMapY -= Delta;
    TileY = (World->TileCountY + (TileY % World->TileCountY)) % World->TileCountY;
  }
  if(TileY >= World->TileCountY) {
    TileMapY += TileY / World->TileCountY;
    TileY = TileY % World->TileCountY;
  }

  canonical_postion Result = {};
  Result.TileMapX = TileMapX;
  Result.TileMapY = TileMapY;
  Result.TileX = TileX;
  Result.TileY = TileY;
  Result.TileRelX = TileRelX;
  Result.TileRelY = TileRelY;

  return Result;
}

bool32
IsWorldEmpty(world *World, raw_position Pos) {
  bool32 Empty = false;
  canonical_postion CanPos = GetCanonicalPosition(World, Pos);

  tile_map *TileMap = GetTileMap(World, CanPos.TileMapX, CanPos.TileMapY);

  if(TileMap) {
    if(CanPos.TileX >= 0 && CanPos.TileX < World->TileCountX && CanPos.TileY >= 0 && CanPos.TileY < World->TileCountY) {
      uint32 TileValue = GetTileValueUnchecked(World, TileMap, CanPos.TileX, CanPos.TileY);
      Empty = TileValue == 0;
    }
  }

  return Empty;
}

extern "C" GAME_UPDATE_VIDEO(GameUpdateVideo) {
  Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == ArrayCount(Input->Controllers[0].Buttons))

  Assert(sizeof(game_state) <= Memory->PermanentStorageSize)
  game_state *State = (game_state *)(Memory->PermanentStorage);

  if(!Memory->IsInitialized) {
    Memory->IsInitialized = true;
    State->PlayerX = 150.0f;
    State->PlayerY = 150.0f;
    State->TileMapX = 0;
    State->TileMapY = 0;
  }

#define TILE_COUNT_X 16
#define TILE_COUNT_Y 9
#define TILE_MAP_COUNT_X 2
#define TILE_MAP_COUNT_Y 2
  uint32 Tiles00[TILE_COUNT_Y][TILE_COUNT_X] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
    {1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0},
    {1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1},
    {1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  };

  uint32 Tiles01[TILE_COUNT_Y][TILE_COUNT_X] = {
    {1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  };

  uint32 Tiles10[TILE_COUNT_Y][TILE_COUNT_X] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  };

  uint32 Tiles11[TILE_COUNT_Y][TILE_COUNT_X] = {
    {1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  };

  world World = {};
  World.TileCountX = TILE_COUNT_X;
  World.TileCountY = TILE_COUNT_Y;
  World.TileMapCountX = TILE_MAP_COUNT_X;
  World.TileMapCountY = TILE_MAP_COUNT_Y;
  World.TileSize = 60.0f;
  World.UpperLeftX = 0.0f;
  World.UpperLeftY = 0.0f;

  tile_map TileMaps[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {};
  TileMaps[0][0].Tiles = (uint32 *)Tiles00;

  tile_map TileMap01 = {};
  TileMaps[0][1].Tiles = (uint32 *)Tiles10;

  tile_map TileMap10 = {};
  TileMaps[1][0].Tiles = (uint32 *)Tiles01;

  tile_map TileMap11 = {};
  TileMaps[1][1].Tiles = (uint32 *)Tiles11;

  World.TileMaps = (tile_map *)TileMaps;

  real32 PlayerSize = 0.75f * World.TileSize;

  tile_map *TileMap = GetTileMap(&World, State->TileMapX, State->TileMapY);

  for(int i = 0; i < ArrayCount(Input->Controllers); i++) {
    game_controller_input *Controller = &Input->Controllers[i];
    if(Controller->IsAnalog) {

    } else {
      real32 dx = 0.0f;
      real32 dy = 0.0f;
      if(Controller->MoveUp.IsEndedDown) {
        dy = -1.0f;
      }
      if(Controller->MoveDown.IsEndedDown) {
        dy = 1.0f;
      }
      if(Controller->MoveLeft.IsEndedDown) {
        dx = -1.0f;
      }
      if(Controller->MoveRight.IsEndedDown) {
        dx = 1.0f;
      }

      dx *= 100;
      dy *= 100;

      real32 NewPlayerX = State->PlayerX + dx * Input->dt;
      real32 NewPlayerY = State->PlayerY + dy * Input->dt;

      raw_position NewPos, NewPosLeft, NewPosRight;
      NewPos.TileMapX = State->TileMapX;
      NewPos.TileMapY = State->TileMapY;
      NewPos.X = NewPlayerX;
      NewPos.Y = NewPlayerY;

      NewPosLeft = NewPos;
      NewPosLeft.X = NewPos.X - PlayerSize * 0.5f;

      NewPosRight = NewPos;
      NewPosRight.X = NewPos.X + PlayerSize * 0.5f;

      if(IsWorldEmpty(&World, NewPos) &&
        IsWorldEmpty(&World, NewPosLeft) &&
        IsWorldEmpty(&World, NewPosRight)) {
        canonical_postion CanPos = GetCanonicalPosition(&World, NewPos);
        State->TileMapX = CanPos.TileMapX;
        State->TileMapY = CanPos.TileMapY;
        State->PlayerX = World.UpperLeftX + (real32)CanPos.TileX * World.TileSize + CanPos.TileRelX;
        State->PlayerY = World.UpperLeftY + (real32)CanPos.TileY * World.TileSize + CanPos.TileRelY;
      }
    }
  }

  // Draw background
  DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height, 0.3f, 0.0f, 0.0f);

  for(int Y = 0; Y < 9; Y++) {
    for(int X = 0; X < 16; X++) {
      real32 TileMinX = World.UpperLeftX + (real32)X * World.TileSize;
      real32 TileMinY = World.UpperLeftY + (real32)Y * World.TileSize;
      real32 TileMaxX = TileMinX + World.TileSize;
      real32 TileMaxY = TileMinY + World.TileSize;
      real32 Gray = 1.0f;
      if(GetTileValueUnchecked(&World, TileMap, X, Y) == 0) {
        Gray = 0.5f;
      }
      DrawRectangle(Buffer, TileMinX, TileMinY, TileMaxX, TileMaxY, Gray, Gray, Gray);
    }
  }

  real32 PlayerMinX = State->PlayerX - 0.5f * PlayerSize;
  real32 PlayerMaxX = State->PlayerX + 0.5f * PlayerSize;
  real32 PlayerMinY = State->PlayerY - PlayerSize;
  real32 PlayerMaxY = State->PlayerY;

  DrawRectangle(Buffer, PlayerMinX, PlayerMinY, PlayerMaxX, PlayerMaxY, 1.0f, 0.0f, 1.0f);
}

extern "C" GAME_UPDATE_AUDIO(GameUpdateAudio) {
}
