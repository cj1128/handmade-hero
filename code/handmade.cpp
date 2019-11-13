#include "handmade.h"

internal int32
RoundReal32ToUint32(real32 Input) {
  uint32 Result = (uint32)(Input + 0.5f);
  return Result;
}

inline int32
TruncateReal32ToInt32(real32 Value) {
  // truncate towards -infinity, not 0
  if(Value < 0) {
    Value -= 1;
  }
  int32 Result =(int32)(Value);
  return Result;
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
IsTileMapEmpty(tile_map *TileMap, real32 PlayerX, real32 PlayerY) {
  bool32 IsEmtpy = false;
  int TileX = TruncateReal32ToInt32((PlayerX - TileMap->Left)/ TileMap->TileSize);
  int TileY = TruncateReal32ToInt32((PlayerY - TileMap->Top)/ TileMap->TileSize);

  if(TileX >= 0 && TileX < TileMap->CountX && TileY >= 0 && TileY < TileMap->CountY) {
    uint32 TileValue = TileMap->Tiles[TileY * TileMap->CountX + TileX];
    if(TileValue == 0) {
      IsEmtpy = true;
    }
  }

  return IsEmtpy;
}

extern "C" GAME_UPDATE_VIDEO(GameUpdateVideo) {
  Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == ArrayCount(Input->Controllers[0].Buttons))

  Assert(sizeof(game_state) <= Memory->PermanentStorageSize)
  game_state *State = (game_state *)(Memory->PermanentStorage);

  if(!Memory->IsInitialized) {
    Memory->IsInitialized = true;
    State->PlayerX = 150.0f;
    State->PlayerY = 150.0f;
  }


#define TILE_MAP_COUNT_X 16
#define TILE_MAP_COUNT_Y 9
  uint32 Tiles[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
    {1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1},
    {1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1},
    {1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1},
    {1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  };

  tile_map TileMap;
  TileMap.CountX = TILE_MAP_COUNT_X;
  TileMap.CountY = TILE_MAP_COUNT_Y;
  TileMap.TileSize = 60;
  TileMap.Tiles = (uint32 *)Tiles;
  TileMap.Left = 0.0f;
  TileMap.Top = 0.0f;

  real32 PlayerSize = 0.75f * TileMap.TileSize;

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

      if(IsTileMapEmpty(&TileMap, NewPlayerX, NewPlayerY) &&
        IsTileMapEmpty(&TileMap, NewPlayerX - PlayerSize * 0.5f, NewPlayerY) &&
        IsTileMapEmpty(&TileMap, NewPlayerX + PlayerSize * 0.5f, NewPlayerY)) {
        State->PlayerX = NewPlayerX;
        State->PlayerY = NewPlayerY;
      }
    }
  }

  for(int Y = 0; Y < 9; Y++) {
    for(int X = 0; X < 16; X++) {
      real32 TileMinX = TileMap.Left + (real32)X * TileMap.TileSize;
      real32 TileMinY = TileMap.Top + (real32)Y * TileMap.TileSize;
      real32 TileMaxX = TileMinX + TileMap.TileSize;
      real32 TileMaxY = TileMinY + TileMap.TileSize;
      real32 Gray = 1.0f;
      if(Tiles[Y][X] == 0) {
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
