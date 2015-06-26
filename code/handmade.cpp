/*
* @Author: dingxijin
* @Date:   2015-04-21 08:09:18
* @Last Modified by:   dingxijin
* @Last Modified time: 2015-06-26 18:58:57
*/

#include "handmade.h"

internal void
UpdateSound(game_sound_buffer *SoundBuffer, int ToneHz)
{
  local_persist float tSine;
  int ToneVolume = 10000;
  int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;
  int16_t *SampleOut = SoundBuffer->Samples;
  for(int SampleIndex = 0 ;
    SampleIndex < SoundBuffer->SampleCount ;
    SampleIndex++)
  {
    float SineValue = sinf(tSine);
    int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
#if 0
    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;
#else
    *SampleOut = 0;
#endif
    tSine += 2.0f * Pi32 / (float)(WavePeriod);
    if( tSine > 2.0f * Pi32 )
    {
      tSine -= 2.0f * Pi32;
    }
  }
}

internal int32
RoundReal32ToInt32(real32 Value)
{
  int32 Result = (int32)(Value + 0.5f);
  return Result;
}

internal uint32
RoundReal32ToUInt32(real32 Value)
{
  uint32 Result = (uint32)(Value + 0.5f);
  return Result;
}

internal void
DrawRect(game_offscreen_buffer *Buffer, real32 MinX, real32 MaxX,
  real32 MinY, real32 MaxY,
  real32 R, real32 G, real32 B)
{
  int32 IntMinX = RoundReal32ToInt32(MinX);
  int32 IntMaxX = RoundReal32ToInt32(MaxX);
  int32 IntMinY = RoundReal32ToInt32(MinY);
  int32 IntMaxY = RoundReal32ToInt32(MaxY);

  uint32 IntR = RoundReal32ToUInt32(R * 255.0f);
  uint32 IntG = RoundReal32ToUInt32(G * 255.0f);
  uint32 IntB = RoundReal32ToUInt32(B * 255.0f);

  uint32 Color = (IntR << 16) |
    (IntG << 8) |
    (IntB);

  if(IntMinX < 0)
  {
    IntMinX = 0;
  }
  if(IntMaxX > Buffer->Width)
  {
    IntMaxX = Buffer->Width;
  }
  if(IntMinY < 0)
  {
    IntMinY = 0;
  }
  if(IntMaxY > Buffer->Height)
  {
    IntMaxY = Buffer->Height;
  }
  for(int Row = IntMinY; Row < IntMaxY; Row++)
  {
    for(int Column = IntMinX; Column < IntMaxX; Column++)
    {
      uint32 *Pixel = (uint32 *)((uint8 *)Buffer->Memory + Row * Buffer->Pitch + Column * Buffer->BytesPerPixel);
      *Pixel = Color;
    }
  }
}

/*
internal void
RenderWeirdGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{
  uint8 *Row = (uint8 *)Buffer->Memory;

  for(int Y = 0;
    Y < Buffer->Height;
    ++Y)
  {
    uint32 *Pixel = (uint32 *)Row;
    for(int XX = 0;
      X < Buffer->Width;
      ++X)
    {
      // Memory Order: BB GG RR XX
      // 0xXXRRGGBB
      uint8 Blue = (uint8)(X + BlueOffset);
      uint8 Green = (uint8)(Y + GreenOffset);

      *Pixel++ = ((Green << 16) | Blue);

    }
    Row += Buffer->Pitch;
  }
}
*/

internal void
RenderPlayer(game_offscreen_buffer *Buffer, int PlayerX, int PlayerY)
{
  uint8 *EndOfBuffer = (uint8 *)Buffer->Memory + Buffer->Pitch * Buffer->Height;

  uint32 Color = 0xABCD12FF;
  int Top = PlayerY;
  int Bottom = PlayerY + 10;
  for(int X = PlayerX;
    X < PlayerX + 10;
    X++)
  {
    uint8 *Pixel = (uint8 *)Buffer->Memory +
      X * Buffer->BytesPerPixel + Buffer->Pitch * Top;
    for(int Y = Top;
      Y <= Bottom;
      Y++)
    {
      if((Pixel >= Buffer->Memory) &&
        ((Pixel + 4) <= EndOfBuffer))
      {
        *(uint32 *)Pixel = Color;
      }
      Pixel += Buffer->Pitch;
    }
  }
}

inline tile_map *
GetTileMap(world *World, int32 X, int32 Y)
{
  Assert(X >= 0 && X < World->TileMapCountX);
  Assert(Y >= 0 && Y < World->TileMapCountY);
  return &World->TileMaps[Y * World->TileMapCountX + X];
}


inline uint32
GetTileValue(world *World, tile_map *TileMap, int32 X, int32 Y)
{
  return TileMap->Tiles[Y * World->TileCountX + X];
}

inline int32
FloorReal32ToInt32(real32 Value)
{
  int32 Result = (int32)(floor(Value));
  return Result;
}


internal canonical_pos
GetCanonicalPos(world *World, raw_pos RawPos)
{
  canonical_pos Pos;
  Pos.TileMapX = RawPos.TileMapX;
  Pos.TileMapY = RawPos.TileMapY;
  Pos.TileX = FloorReal32ToInt32((RawPos.X - World->UpperLeftX) / World->TileWidth);
  Pos.TileY = FloorReal32ToInt32((RawPos.Y - World->UpperLeftY) / World->TileHeight);

  Pos.X = RawPos.X - World->TileWidth * Pos.TileX - World->UpperLeftX;
  Pos.Y = RawPos.Y - World->TileHeight * Pos.TileY - World->UpperLeftY;

  Assert(Pos.X >= 0 && Pos.X < World->TileWidth);
  Assert(Pos.Y >= 0 && Pos.Y < World->TileHeight);

  if(Pos.TileX < 0)
  {
    --Pos.TileMapX;
    Pos.TileX += World->TileCountX;
  }
  if(Pos.TileX >= World->TileCountX)
  {
    ++Pos.TileMapX;
    Pos.TileX -= World->TileCountX;
  }
  if(Pos.TileY < 0)
  {
    --Pos.TileMapY;
    Pos.TileY += World->TileCountY;
  }
  if(Pos.TileY >= World->TileCountY)
  {
    ++Pos.TileMapY;
    Pos.TileY -= World->TileCountY;
  }
  return Pos;
}

internal bool
IsWorldPointValid(world *World, raw_pos TestPos)
{
  bool Result = false;
  canonical_pos Pos = GetCanonicalPos(World, TestPos);
  tile_map *TileMap = GetTileMap(World, Pos.TileMapX, Pos.TileMapY);

  if(GetTileValue(World, TileMap, Pos.TileX, Pos.TileY) == 0)
  {
    Result = true;
  }
  return Result;
}


extern "C" GAME_UPDATE_VIDEO(GameUpdateVideo)
{

  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
  game_state *GameState = (game_state *)Memory->PermanentStorage;
  if(!Memory->IsInitialized)
  {
    Memory->IsInitialized = true;
    GameState->PlayerX = 150;
    GameState->PlayerY = 150;
    GameState->TileMapX = 0;
    GameState->TileMapY = 0;
  }

  world World = {};
  tile_map TileMaps[2][2] = {};

  World.UpperLeftX = 30;
  World.UpperLeftY = 30;
  World.TileWidth = 40;
  World.TileHeight = 50;
#define TILE_MAP_TILE_COUNT_X 18
#define TILE_MAP_TILE_COUNT_Y 10
  World.TileCountX = TILE_MAP_TILE_COUNT_X;
  World.TileCountY = TILE_MAP_TILE_COUNT_Y;
  World.TileMapCountX = 2;
  World.TileMapCountY = 2;

  real32 PlayerWidth = 0.75f * World.TileWidth;
  real32 PlayerHeight = World.TileHeight;

  uint32 Tile00[TILE_MAP_TILE_COUNT_Y][TILE_MAP_TILE_COUNT_X] =
  {
    {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1},
    {1, 0, 0, 1,  1, 1, 1, 1,  0, 0, 0, 0,  0, 0, 1, 0,  0, 1},
    {1, 0, 0, 0,  0, 1, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1},
    {1, 0, 0, 0,  0, 0, 0, 1,  0, 0, 0, 0,  0, 1, 1, 0,  0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1},
    {1, 0, 0, 0,  0, 1, 0, 1,  0, 0, 0, 0,  1, 0, 0, 0,  0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1},
    {1, 0, 0, 0,  0, 1, 0, 0,  1, 1, 1, 1,  1, 0, 0, 0,  0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0},
    {1, 1, 1, 1,  1, 1, 1, 1,  1, 0, 1, 1,  1, 1, 1, 1,  1, 1},
  };
  uint32 Tile01[TILE_MAP_TILE_COUNT_Y][TILE_MAP_TILE_COUNT_X] =
  {
    {1, 1, 1, 1,  1, 1, 1, 1,  1, 0, 1, 1,  1, 1, 1, 1,  1, 1},
    {1, 1, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 1, 0,  0, 1},
    {1, 1, 0, 0,  0, 1, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1},
    {1, 1, 0, 0,  0, 0, 0, 1,  0, 0, 0, 0,  0, 1, 1, 0,  0, 1},
    {1, 1, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0},
    {1, 0, 0, 0,  0, 1, 0, 1,  0, 0, 0, 0,  1, 0, 0, 0,  0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1},
    {1, 0, 0, 0,  0, 1, 0, 0,  1, 1, 1, 1,  1, 0, 0, 0,  0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1},
    {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1},
  };
  uint32 Tile10[TILE_MAP_TILE_COUNT_Y][TILE_MAP_TILE_COUNT_X] =
  {
    {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1, 1, 1, 1,  0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1},
    {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1},
    {1, 1, 1, 1,  1, 1, 1, 1,  1, 0, 1, 1,  1, 1, 1, 1,  1, 1},
  };
  uint32 Tile11[TILE_MAP_TILE_COUNT_Y][TILE_MAP_TILE_COUNT_X] =
  {
    {1, 1, 1, 1,  1, 1, 1, 1,  1, 0, 1, 1,  1, 1, 1, 1,  1, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 1, 0,  0, 1},
    {1, 0, 0, 0,  0, 1, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1},
    {1, 0, 0, 0,  0, 0, 0, 1,  0, 0, 0, 0,  0, 1, 1, 0,  0, 1},
    {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1},
    {1, 0, 0, 0,  0, 1, 0, 1,  0, 0, 0, 0,  1, 0, 0, 0,  0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1},
    {1, 0, 0, 0,  0, 1, 0, 0,  1, 1, 1, 1,  1, 0, 0, 0,  0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1},
    {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1},
  };

  TileMaps[0][0].Tiles = (uint32 *)Tile00;
  TileMaps[0][1].Tiles = (uint32 *)Tile10;
  TileMaps[1][0].Tiles = (uint32 *)Tile01;
  TileMaps[1][1].Tiles = (uint32 *)Tile11;

  World.TileMaps = (tile_map *)TileMaps;

  for(int Index = 0 ;
    Index < ArrayCount(Input->Controllers) && Input->Controllers[Index].IsConnected;
    Index++)
  {
    game_controller_input *Controller = GetController(Input, Index);
    if(Controller->IsAnalog)
    {
    }
    else
    {
      int dx = 0;
      int dy = 0;
      if(Controller->MoveLeft.EndedDown)
      {
        dx = -1;
      }
      else if(Controller->MoveRight.EndedDown)
      {
        dx = 1;
      }
      else if(Controller->MoveUp.EndedDown)
      {
        dy = -1;
      }
      else if(Controller->MoveDown.EndedDown)
      {
        dy = 1;
      }
      dx *= 100;
      dy *= 100;

      real32 NewPlayerX = GameState->PlayerX + Input->TimeForFrame * dx;
      real32 NewPlayerY = GameState->PlayerY + Input->TimeForFrame * dy;
      raw_pos TestPos = {};
      TestPos.TileMapX = GameState->TileMapX;
      TestPos.TileMapY = GameState->TileMapY;
      TestPos.X = NewPlayerX;
      TestPos.Y = NewPlayerY;
      raw_pos TestPosLeft = TestPos;
      TestPosLeft.X -= PlayerWidth / 2;

      raw_pos TestPostRight = TestPos;
      TestPostRight.X += PlayerWidth / 2;

      if(IsWorldPointValid(&World, TestPos) &&
        IsWorldPointValid(&World, TestPosLeft) &&
        IsWorldPointValid(&World, TestPostRight))
      {
        canonical_pos Pos = GetCanonicalPos(&World, TestPos);
        GameState->TileMapX = Pos.TileMapX;
        GameState->TileMapY = Pos.TileMapY;
        GameState->PlayerX = World.UpperLeftX + Pos.TileX * World.TileWidth + Pos.X;
        GameState->PlayerY = World.UpperLeftY + Pos.TileY * World.TileHeight + Pos.Y;
      }
    }
  }
  tile_map *TileMap = GetTileMap(&World, GameState->TileMapX, GameState->TileMapY);



  DrawRect(Buffer, 0.0f, (real32)Buffer->Width, 0.0f, (real32)Buffer->Height, 0.7f, 0.9f, 1.0f);


  for(int Row = 0; Row < TILE_MAP_TILE_COUNT_Y; Row++)
  {
    for(int Column = 0; Column < TILE_MAP_TILE_COUNT_X; Column++)
    {
      real32 gray = 0.5;
      if(GetTileValue(&World, TileMap, Column, Row))
      {
        gray = 1.0;
      }
      real32 MinX = Column * World.TileWidth + World.UpperLeftX;
      real32 MinY = Row * World.TileHeight + World.UpperLeftY;
      real32 MaxX = MinX + World.TileWidth;
      real32 MaxY = MinY + World.TileHeight;
      DrawRect(Buffer, MinX, MaxX, MinY, MaxY,
        gray, gray, gray
      );
    }
  }
  real32 PX = GameState -> PlayerX;
  real32 PY = GameState -> PlayerY;
  DrawRect(Buffer, PX - PlayerWidth / 2, PX + PlayerWidth / 2, PY - PlayerHeight, PY, 0.7f, 0.8f, 0.9f);
}

extern "C" GAME_UPDATE_AUDIO(GameUpdateAudio)
{
  game_state *GameState = (game_state *)Memory->PermanentStorage;
  UpdateSound(SoundBuffer, 40);
}
