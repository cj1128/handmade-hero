/*
* @Author: dingxijin
* @Date:   2015-04-21 08:09:18
* @Last Modified by:   dingxijin
* @Last Modified time: 2015-07-23 12:27:39
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

#define PushSize(Arena, Type) (Type *)PushSize_(Arena, sizeof(Type))
#define PushArray(Arena, Count, Type) (Type *)PushSize_(Arena, (Count) * sizeof(Type))

internal uint8 *
PushSize_(memory_arena *Arena, uint32 Size)
{
  Assert(Arena->Used + Size <= Arena->Size);
  uint8 *Result = Arena->Base + Arena->Used;
  Arena->Used += Size;
  return Result;
}


extern "C" GAME_UPDATE_VIDEO(GameUpdateVideo)
{

  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
  game_state *GameState = (game_state *)Memory->PermanentStorage;
  if(!Memory->IsInitialized)
  {
    Memory->IsInitialized = true;

    GameState->PlayerP.AbsTileX = 3;
    GameState->PlayerP.AbsTileY = 3;
    GameState->PlayerP.TileRelX = 1.0f;
    GameState->PlayerP.TileRelY = 1.0f;

    memory_arena WorldArena = {};
    WorldArena.Base = (uint8 *)Memory->PermanentStorage + sizeof(game_state);
    WorldArena.Size = Memory->PermanentStorageSize - sizeof(game_state);
    WorldArena.Used = 0;

    GameState->World = PushSize(&WorldArena, world);
    world *World = GameState->World;
    World->TileMap = PushSize(&WorldArena, tile_map);

    tile_map *TileMap = World->TileMap;

    TileMap->ChunkShift = 8;
    TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
    TileMap->ChunkDim = 1 << TileMap->ChunkShift;

    TileMap->TileChunkCountX = 10;
    TileMap->TileChunkCountY = 10;

    TileMap->TileSideInMeters = 1.4f;
    TileMap->TileSideInPixels = 45;
    TileMap->MetersToPixels = TileMap->TileSideInPixels / TileMap->TileSideInMeters;

    TileMap->TileChunks = PushArray(&WorldArena, TileMap->TileChunkCountY * TileMap->TileChunkCountX, tile_chunk);

    for(uint32 Y = 0; Y < TileMap->TileChunkCountY; Y++)
    {
      for(uint32 X = 0; X < TileMap->TileChunkCountX; X++)
      {
        TileMap->TileChunks[Y * TileMap->TileChunkCountX + X].Tiles = PushArray(&WorldArena, TileMap->ChunkDim * TileMap->ChunkDim, uint32);
      }
    }

    int ScreenXCount = 20;
    int ScreenYCount = 20;
    int TilePerWidth = 20;
    int TilePerHeight = 20;

    for(int ScreenX = 0; ScreenX < ScreenXCount; ScreenX++)
    {
      for(int ScreenY = 0; ScreenY < ScreenYCount; ScreenY++)
      {
        for(int TileX = 0; TileX < TilePerWidth; TileX++)
        {
          for(int TileY = 0; TileY < TilePerHeight; TileY++)
          {
            uint32 TileValue = (TileX == (TileY + 1)) ? 1 : 0;
            SetTileValue(TileMap, ScreenX * ScreenXCount + TileX,
              ScreenY * ScreenYCount + TileY, TileValue);
          }
        }
      }
    }
  }

  world *World = GameState->World;
  tile_map *TileMap = World->TileMap;

  int32 UpperLeftX = 30;
  int32 UpperLeftY = 30;

  real32 PlayerHeight = 1.4f;
  real32 PlayerWidth = 0.75f * PlayerHeight;

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
      int Speed = 2;
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

      if(Controller->ActionUp.EndedDown)
      {
        Speed = 10;
      }
      dx *= Speed;
      dy *= Speed;

      tile_map_pos NewPlayerP = GameState->PlayerP;
      NewPlayerP.TileRelX += Input->TimeForFrame * dx;
      NewPlayerP.TileRelY += Input->TimeForFrame * dy;
      NewPlayerP = RecanonicalizePos(TileMap, NewPlayerP);

      tile_map_pos LeftPlayerP = NewPlayerP;
      LeftPlayerP.TileRelX -= PlayerWidth / 2;
      LeftPlayerP = RecanonicalizePos(TileMap, LeftPlayerP);

      tile_map_pos RightPlayerP = NewPlayerP;
      RightPlayerP.TileRelX += PlayerWidth / 2;
      RightPlayerP = RecanonicalizePos(TileMap, RightPlayerP);

      if(IsWorldPointValid(TileMap, NewPlayerP) &&
        IsWorldPointValid(TileMap, LeftPlayerP) &&
        IsWorldPointValid(TileMap, RightPlayerP))
      {
        GameState->PlayerP = NewPlayerP;
      }
    }
  }
  // tile_chunk *TileChunk = GetTileChunk(&TileMap, GameState->PlayerP.AbsTileX, GameState->PlayerP.AbsTileY);

  DrawRect(Buffer, 0.0f, (real32)Buffer->Width, 0.0f, (real32)Buffer->Height, 0.7f, 0.9f, 1.0f);

  real32 CenterX = (real32)Buffer->Width / 2;
  real32 CenterY = (real32)Buffer->Height / 2;

  for(int RelRow = -10; RelRow < 10; RelRow++)
  {
    for(int RelColumn = -10; RelColumn < 10; RelColumn++)
    {
      real32 gray = 0.5;
      uint32 Row = GameState->PlayerP.AbsTileY + RelRow;
      uint32 Column = GameState->PlayerP.AbsTileX + RelColumn;
      if(GetTileValue(TileMap, Column, Row))
      {
        gray = 1.0;
      }

     if(Row == GameState->PlayerP.AbsTileY && Column == GameState->PlayerP.AbsTileX)
      {
       gray = 0.0;
      }

      real32 TileCenX = CenterX + (RelColumn * TileMap->TileSideInPixels) - TileMap->MetersToPixels * GameState->PlayerP.TileRelX;
      real32 TileCenY = CenterY + (RelRow * TileMap->TileSideInPixels) - TileMap->MetersToPixels * GameState->PlayerP.TileRelY;
      real32 MinX = TileCenX - 0.5f * TileMap->TileSideInPixels;
      real32 MinY = TileCenY - 0.5f * TileMap->TileSideInPixels;
      real32 MaxX = MinX + TileMap->TileSideInPixels;
      real32 MaxY = MinY + TileMap->TileSideInPixels;

      DrawRect(Buffer, MinX, MaxX, MinY, MaxY,
        gray, gray, gray
      );
    }
  }
  real32 PlayerMinX = CenterX + TileMap->MetersToPixels * GameState->PlayerP.TileRelX - 0.5f * TileMap->MetersToPixels * PlayerWidth;
  real32 PlayerMinY = CenterY + TileMap->MetersToPixels * GameState->PlayerP.TileRelY - 0.5f * TileMap->MetersToPixels * PlayerHeight;
  DrawRect(Buffer, PlayerMinX, PlayerMinX + PlayerWidth * TileMap->MetersToPixels, PlayerMinY, PlayerMinY + PlayerHeight * TileMap->MetersToPixels, 0.7f, 0.8f, 0.9f);
}

extern "C" GAME_UPDATE_AUDIO(GameUpdateAudio)
{
  game_state *GameState = (game_state *)Memory->PermanentStorage;
  UpdateSound(SoundBuffer, 40);
}
