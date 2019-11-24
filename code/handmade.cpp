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

internal inline tile_map_position
GetTileMapPosition(world *World, uint32 AbsTileX, uint32 AbsTileY) {
  tile_map_position Result = {};

  Result.TileMapX = AbsTileX >> World->TileMapShift;
  Result.RelTileX = AbsTileX & World->TileMapMask;

  Result.TileMapY = AbsTileY >> World->TileMapShift;
  Result.RelTileY = AbsTileY & World->TileMapMask;

  return Result;
}

inline tile_map *
GetTileMap(world *World, uint32 TileMapX, uint32 TileMapY) {
  tile_map *Result = NULL;

  if(TileMapX < World->TileMapCountX && TileMapY < World->TileMapCountY) {
    Result = &World->TileMaps[TileMapY * World->TileMapDim + TileMapX];
  }

  return Result;
}

internal inline void
RecononicalizeCoord(world *World, uint32 *Tile, real32 *TileRel) {
  // NOTE: world is assumed to be toroidal
  int32 Offset = FloorReal32ToInt32(*TileRel / World->TileSizeInMeters);
  *Tile += Offset;
  *TileRel -= Offset * World->TileSizeInMeters;

  Assert(*TileRel >= 0);
  Assert(*TileRel < World->TileSizeInMeters);
}

inline uint32
GetTileValue(world *World, uint32 AbsTileX, uint32 AbsTileY) {
  uint32 Result = 2;

  tile_map_position P = GetTileMapPosition(World, AbsTileX, AbsTileY);
  tile_map *TileMap = GetTileMap(World, P.TileMapX, P.TileMapY);

  if(TileMap) {
    Result = TileMap->Tiles[P.RelTileY * World->TileMapDim + P.RelTileX];
  }

  return Result;
}

internal inline world_position
RecononicalizePosition(world *World, world_position Pos) {
  world_position Result = Pos;
  RecononicalizeCoord(World, &Result.AbsTileX, &Result.TileRelX);
  RecononicalizeCoord(World, &Result.AbsTileY, &Result.TileRelY);
  return Result;
}

bool32
IsWorldEmpty(world *World, world_position Pos) {
  bool32 Empty = false;

  Empty = GetTileValue(World, Pos.AbsTileX, Pos.AbsTileY) == 0;

  return Empty;
}

extern "C" GAME_UPDATE_VIDEO(GameUpdateVideo) {
  Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == ArrayCount(Input->Controllers[0].Buttons))

  Assert(sizeof(game_state) <= Memory->PermanentStorageSize)
  game_state *State = (game_state *)(Memory->PermanentStorage);

  if(!Memory->IsInitialized) {
    Memory->IsInitialized = true;

    State->PlayerPos = {};
    State->PlayerPos.AbsTileX = 3;
    State->PlayerPos.AbsTileY = 3;
    State->PlayerPos.TileRelX = 0.5f;
    State->PlayerPos.TileRelY = 0.5f;
  }

#define TILE_COUNT_X 256
#define TILE_COUNT_Y 256
#define TILE_MAP_COUNT_X 2
#define TILE_MAP_COUNT_Y 2
  uint32 Tiles[TILE_COUNT_Y][TILE_COUNT_X] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  };

  real32 CenterX = (real32)Buffer->Width / 2;
  real32 CenterY = (real32)Buffer->Height / 2;

  world World = {};
  World.TileMapShift = 8;
  World.TileMapDim = 1 << World.TileMapShift;
  World.TileMapMask = World.TileMapDim - 1;
  World.TileMapCountX = 1;
  World.TileMapCountY = 1;
  World.TileSizeInMeters = 1.4f;
  World.TileSizeInPixels = 55.0f;
  World.MetersToPixels = World.TileSizeInPixels / World.TileSizeInMeters;

  tile_map TileMaps[1];
  TileMaps[0].Tiles = (uint32 *)Tiles;
  World.TileMaps = (tile_map *)TileMaps;

  real32 PlayerHeight = 1.4f;
  real32 PlayerWidth = 0.75f * PlayerHeight;

  for(int i = 0; i < ArrayCount(Input->Controllers); i++) {
    game_controller_input *Controller = &Input->Controllers[i];
    if(Controller->IsAnalog) {

    } else {
      real32 dx = 0.0f;
      real32 dy = 0.0f;
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

      dx *= 2.0f;
      dy *= 2.0f;

      world_position NewPos = State->PlayerPos;

      NewPos.TileRelX += dx * Input->dt;
      NewPos.TileRelY += dy * Input->dt;
      NewPos = RecononicalizePosition(&World, NewPos);

      world_position NewPosLeft = NewPos;
      NewPosLeft.TileRelX -= PlayerWidth * 0.5f;
      NewPosLeft = RecononicalizePosition(&World, NewPosLeft);

      world_position NewPosRight = NewPos;
      NewPosRight.TileRelX += PlayerWidth * 0.5f;
      NewPosRight = RecononicalizePosition(&World, NewPosRight);

      if(IsWorldEmpty(&World, NewPos) &&
        IsWorldEmpty(&World, NewPosLeft) &&
        IsWorldEmpty(&World, NewPosRight)) {
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

      real32 TileMinX = CenterX + (real32)RelX * World.TileSizeInPixels;
      real32 TileMaxX = TileMinX + World.TileSizeInPixels;

      real32 TileMinY = CenterY + (real32)RelY * World.TileSizeInPixels;
      real32 TileMaxY = TileMinY + World.TileSizeInPixels;

      uint32 AbsX = RelX + State->PlayerPos.AbsTileX;
      uint32 AbsY = RelY + State->PlayerPos.AbsTileY;

      uint32 V = GetTileValue(&World, AbsX, AbsY);

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

  real32 PlayerMinX = CenterX + World.MetersToPixels * State->PlayerPos.TileRelX - 0.5f * PlayerWidth * World.MetersToPixels;
  real32 PlayerMaxX = PlayerMinX + PlayerWidth * World.MetersToPixels;
  real32 PlayerMinY = CenterY + World.MetersToPixels * State->PlayerPos.TileRelY;
  real32 PlayerMaxY = PlayerMinY + PlayerHeight * World.MetersToPixels;

  DrawRectangle(Buffer, PlayerMinX, H - PlayerMaxY, PlayerMaxX, H - PlayerMinY, 1.0f, 0.0f, 1.0f);
}

extern "C" GAME_UPDATE_AUDIO(GameUpdateAudio) {
}
