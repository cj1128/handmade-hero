#include "handmade.h"

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

internal inline void
RecononicalizeCoord(world *World, int32 TileCount, int32 *TileMap, int32 *Tile, real32 *TileRel) {
  int32 Offset = FloorReal32ToInt32(*TileRel / World->TileSizeInMeters);
  *Tile += Offset;
  *TileRel -= Offset * World->TileSizeInMeters;

  Assert(*TileRel >= 0);
  Assert(*TileRel < World->TileSizeInMeters);

  if(*Tile < 0) {
    int32 Delta = (-*Tile / TileCount) + 1;
    *TileMap -= Delta;
    *Tile = (TileCount + (*Tile % TileCount)) % TileCount;
  }

  if(*Tile >= TileCount) {
    *TileMap += *Tile / TileCount;
    *Tile = *Tile % TileCount;
  }
}

internal inline canonical_postion
RecononicalizePosition(world *World, canonical_postion Pos) {
  canonical_postion Result = Pos;
  RecononicalizeCoord(World, World->TileCountX, &Result.TileMapX, &Result.TileX, &Result.TileRelX);
  RecononicalizeCoord(World, World->TileCountY, &Result.TileMapY, &Result.TileY, &Result.TileRelY);
  return Result;
}

bool32
IsWorldEmpty(world *World, canonical_postion Pos) {
  bool32 Empty = false;

  tile_map *TileMap = GetTileMap(World, Pos.TileMapX, Pos.TileMapY);

  if(TileMap) {
    if(Pos.TileX >= 0 && Pos.TileX < World->TileCountX && Pos.TileY >= 0 && Pos.TileY < World->TileCountY) {
      uint32 TileValue = GetTileValueUnchecked(World, TileMap, Pos.TileX, Pos.TileY);
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

    State->PlayerPos = {};
    State->PlayerPos.TileMapX = 0;
    State->PlayerPos.TileMapY = 0;
    State->PlayerPos.TileX = 3;
    State->PlayerPos.TileY = 3;
    State->PlayerPos.TileRelX = 0.5f;
    State->PlayerPos.TileRelY = 0.5f;
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
  World.TileSizeInMeters = 1.4f;
  World.TileSizeInPixels = 55.0f;
  World.MetersToPixels = World.TileSizeInPixels / World.TileSizeInMeters;
  World.UpperLeftX = 10.0f;
  World.UpperLeftY = 10.0f;

  tile_map TileMaps[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {};
  TileMaps[0][0].Tiles = (uint32 *)Tiles00;

  tile_map TileMap01 = {};
  TileMaps[0][1].Tiles = (uint32 *)Tiles10;

  tile_map TileMap10 = {};
  TileMaps[1][0].Tiles = (uint32 *)Tiles01;

  tile_map TileMap11 = {};
  TileMaps[1][1].Tiles = (uint32 *)Tiles11;

  World.TileMaps = (tile_map *)TileMaps;

  real32 PlayerHeight = 1.4f;
  real32 PlayerWidth = 0.75f * PlayerHeight;

  tile_map *TileMap = GetTileMap(&World, State->PlayerPos.TileMapX, State->PlayerPos.TileMapY);

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

      dx *= 2.0f;
      dy *= 2.0f;

      canonical_postion NewPos = State->PlayerPos;

      NewPos.TileRelX += dx * Input->dt;
      NewPos.TileRelY += dy * Input->dt;
      NewPos = RecononicalizePosition(&World, NewPos);

      canonical_postion NewPosLeft = NewPos;
      NewPosLeft.TileRelX -= PlayerWidth * 0.5f;
      NewPosLeft = RecononicalizePosition(&World, NewPosLeft);

      canonical_postion NewPosRight = NewPos;
      NewPosRight.TileRelX += PlayerWidth * 0.5f;
      NewPosRight = RecononicalizePosition(&World, NewPosRight);

      if(IsWorldEmpty(&World, NewPos) &&
        IsWorldEmpty(&World, NewPosLeft) &&
        IsWorldEmpty(&World, NewPosRight)) {
        State->PlayerPos = NewPos;
      }
    }
  }

  // Draw background
  DrawRectangle(Buffer, 0.0f, 0.0f, (real32)Buffer->Width, (real32)Buffer->Height, 0.4f, 0.0f, 0.0f);

  for(int Y = 0; Y < 9; Y++) {
    for(int X = 0; X < 16; X++) {
      real32 TileMinX = World.UpperLeftX + (real32)X * World.TileSizeInPixels;
      real32 TileMinY = World.UpperLeftY + (real32)Y * World.TileSizeInPixels;
      real32 TileMaxX = TileMinX + World.TileSizeInPixels;
      real32 TileMaxY = TileMinY + World.TileSizeInPixels;
      real32 Gray = 1.0f;

      if(GetTileValueUnchecked(&World, TileMap, X, Y) == 0) {
        Gray = 0.5f;
      }

      if(Y == State->PlayerPos.TileY && X == State->PlayerPos.TileX) {
        Gray = 0.0f;
      }

      DrawRectangle(Buffer, TileMinX, TileMinY, TileMaxX, TileMaxY, Gray, Gray, Gray);
    }
  }

  real32 PlayerMinX = World.UpperLeftX + World.TileSizeInPixels * State->PlayerPos.TileX + World.MetersToPixels * State->PlayerPos.TileRelX - 0.5f * PlayerWidth * World.MetersToPixels;
  real32 PlayerMaxX = PlayerMinX + PlayerWidth * World.MetersToPixels;
  real32 PlayerMinY = World.UpperLeftY + World.TileSizeInPixels * State->PlayerPos.TileY + World.MetersToPixels * State->PlayerPos.TileRelY - PlayerHeight * World.MetersToPixels;
  real32 PlayerMaxY = PlayerMinY + PlayerHeight * World.MetersToPixels;

  DrawRectangle(Buffer, PlayerMinX, PlayerMinY, PlayerMaxX, PlayerMaxY, 1.0f, 0.0f, 1.0f);
}

extern "C" GAME_UPDATE_AUDIO(GameUpdateAudio) {
}
