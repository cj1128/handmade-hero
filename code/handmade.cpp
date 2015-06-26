/*
* @Author: dingxijin
* @Date:   2015-04-21 08:09:18
* @Last Modified by:   dingxijin
* @Last Modified time: 2015-06-26 13:49:22
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

internal bool
IsTileMapPointValid(tile_map *TileMap, real32 TestX, real32 TestY)
{
  bool Result = false;
  int32 TileX = (int32)((TestX - TileMap->UpperLeftX) / TileMap->TileWidth);
  int32 TileY = (int32)((TestY - TileMap->UpperLeftY) / TileMap->TileHeight);
  if(TileX >= 0 && TileX < TileMap->CountX &&
    TileY >= 0 && TileY < TileMap->CountY &&
    TileMap->Tiles[TileY * TileMap->CountX + TileX] != 1)
  {
    Result = true;
  }
  return Result;
}

inline uint32
GetTileValue(tile_map *TileMap, int32 Row, int32 Column)
{
  return TileMap->Tiles[Row * TileMap->CountX + Column];
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
  }

  tile_map TileMaps[2][2] = {};

  TileMaps[0][0].UpperLeftX = 30;
  TileMaps[0][0].UpperLeftY = 30;
  TileMaps[0][0].TileWidth = 50;
  TileMaps[0][0].TileHeight = 50;
#define TILE_MAP_COUNT_X 20
#define TILE_MAP_COUNT_Y 10
  TileMaps[0][0].CountX = TILE_MAP_COUNT_X;
  TileMaps[0][0].CountY = TILE_MAP_COUNT_Y;

  real32 PlayerWidth = 40;
  real32 PlayerHeight = 40;

  uint32 Tile00[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
  {
    {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 1, 0,  0, 1},
    {1, 0, 0, 0,  0, 1, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1},
    {1, 0, 0, 0,  0, 0, 0, 1,  0, 0, 0, 0,  0, 1, 1, 0,  0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1},
    {1, 0, 0, 0,  0, 1, 0, 1,  0, 0, 0, 0,  1, 0, 0, 0,  0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1},
    {1, 0, 0, 0,  0, 1, 0, 0,  1, 1, 1, 1,  1, 0, 0, 0,  0, 1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 1},
    {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1},
  };

  TileMaps[0][0].Tiles = (uint32 *)Tile00;
  tile_map *TileMap = &TileMaps[0][0];


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
      dx *= 60;
      dy *= 60;

      real32 NewPlayerX = GameState->PlayerX + Input->TimeForFrame * dx;
      real32 NewPlayerY = GameState->PlayerY + Input->TimeForFrame * dy;

      if(IsTileMapPointValid(TileMap, NewPlayerX, NewPlayerY) &&
        IsTileMapPointValid(TileMap, NewPlayerX - PlayerWidth / 2, NewPlayerY) &&
        IsTileMapPointValid(TileMap, NewPlayerX - PlayerWidth / 2, NewPlayerY - PlayerHeight) &&
        IsTileMapPointValid(TileMap, NewPlayerX + PlayerWidth / 2, NewPlayerY - PlayerHeight) &&
        IsTileMapPointValid(TileMap, NewPlayerX + PlayerWidth / 2, NewPlayerY))
      {
        GameState->PlayerX = NewPlayerX;
        GameState->PlayerY = NewPlayerY;
      }
    }
  }



  DrawRect(Buffer, 0.0f, (real32)Buffer->Width, 0.0f, (real32)Buffer->Height, 0.7f, 0.9f, 1.0f);


  for(int Row = 0; Row < TILE_MAP_COUNT_Y; Row++)
  {
    for(int Column = 0; Column < TILE_MAP_COUNT_X; Column++)
    {
      real32 gray = 0.5;
      if(GetTileValue(TileMap, Row, Column))
      {
        gray = 1.0;
      }
      real32 MinX = Column * TileMap->TileWidth + TileMap->UpperLeftX;
      real32 MinY = Row * TileMap->TileHeight + TileMap->UpperLeftY;
      real32 MaxX = MinX + TileMap->TileWidth;
      real32 MaxY = MinY + TileMap->TileHeight;
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
