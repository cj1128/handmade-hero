/*
* @Author: dingxijin
* @Date:   2015-04-21 08:09:18
* @Last Modified by:   dingxijin
* @Last Modified time: 2015-07-27 00:23:22
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




extern "C" GAME_UPDATE_VIDEO(GameUpdateVideo)
{

  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
  game_state *GameState = (game_state *)Memory->PermanentStorage;



  if(!Memory->IsInitialized)
  {
    Memory->IsInitialized = true;

    GameState->PlayerP.AbsTileX = 3;
    GameState->PlayerP.AbsTileY = 3;
    GameState->PlayerP.AbsTileZ = 0;
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

    TileMap->ChunkShift = 4;
    TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
    TileMap->ChunkDim = 1 << TileMap->ChunkShift;

    TileMap->TileChunkCountX = 64;
    TileMap->TileChunkCountY = 64;
    TileMap->TileChunkCountZ = 2;

    TileMap->TileSideInMeters = 1.4f;

    TileMap->TileChunks = PushArray(&WorldArena, TileMap->TileChunkCountY * TileMap->TileChunkCountX * TileMap->TileChunkCountZ, tile_chunk);

    int TilePerWidth = 19;
    int TilePerHeight = 7;

    int ScreenX = 0;
    int ScreenY = 0;

    bool DoorUp = false;
    bool DoorDown = false;
    bool DoorTop = false;
    bool DoorBottom = false;
    bool DoorLeft = false;
    bool DoorRight = false;

    uint32 AbsTileZ = 0;

    for(int ScreenIndex = 0; ScreenIndex < 20; ScreenIndex++)
    {
      int RandomChoice;
      if(DoorUp || DoorDown)
      {
        RandomChoice = rand() % 2;
      }
      else
      {
        RandomChoice = rand() % 3;
      }
      if(RandomChoice == 2)
      {
        if(AbsTileZ == 0)
        {
          DoorUp = true;
        }
        else if(AbsTileZ == 1)
        {
          DoorDown = true;
        }
      }
      if(RandomChoice == 0)
      {
        DoorRight = true;
      }
      if(RandomChoice == 1)
      {
        DoorBottom = true;
      }

      for(int TileY = 0; TileY < TilePerHeight; TileY++)
      {
        for(int TileX = 0; TileX < TilePerWidth; TileX++)
        {
          uint32 TileValue = 1;
          if(TileX == 0 && (!DoorLeft || (TileY != (TilePerHeight / 2))) )
          {
            TileValue = 2;
          }
          if(TileX == (TilePerWidth -1) && (!DoorRight || (TileY != (TilePerHeight / 2))))
          {
            TileValue = 2;
          }
          if(TileY == 0 && (!DoorTop || (TileX != (TilePerWidth / 2))))
          {
            TileValue = 2;
          }
          if(TileY == (TilePerHeight -1) && (!DoorBottom || (TileX != (TilePerWidth /2))))
          {
            TileValue = 2;
          }
          if(TileX == 10 && TileY == 5)
          {
            if(DoorUp)
            {
              TileValue = 3;
            }
            if(DoorDown)
            {
              TileValue = 4;
            }
          }
          SetTileValue(&WorldArena, TileMap, ScreenX * TilePerWidth + TileX,
            ScreenY * TilePerHeight + TileY, AbsTileZ, TileValue);
        }
      }

      DoorLeft = DoorRight;
      DoorTop = DoorBottom;

      DoorRight = false;
      DoorBottom = false;

      if(DoorUp)
      {
        DoorDown = true;
        DoorUp = false;
        AbsTileZ = 1;
      }
      else if(DoorDown)
      {
        DoorUp = true;
        DoorDown = false;
        AbsTileZ = 0;
      }

      if(RandomChoice == 0)
      {
        ScreenX += 1;
      }
      else
      {
        ScreenY += 1;
      }
    }
  }

  world *World = GameState->World;
  tile_map *TileMap = World->TileMap;

  real32 TileSideInPixels = 60;
  real32 MetersToPixels = TileSideInPixels / TileMap->TileSideInMeters;

  int32 UpperLeftX = 30;
  int32 UpperLeftY = 30;

  real32 PlayerHeight = 1.0f;
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
        dy = 1;
      }
      else if(Controller->MoveDown.EndedDown)
      {
        dy = -1;
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

  DrawRect(Buffer, 0.0f, (real32)Buffer->Width, 0.0f, (real32)Buffer->Height, 1.0f, 0.0f, 0.0f);

  real32 ScreenCenterX = (real32)Buffer->Width / 2;
  real32 ScreenCenterY = (real32)Buffer->Height / 2;

  for(int RelRow = -10; RelRow < 100; RelRow++)
  {
    for(int RelColumn = -10; RelColumn < 100; RelColumn++)
    {
      real32 gray = 0.5;
      uint32 Row = GameState->PlayerP.AbsTileY + RelRow;
      uint32 Column = GameState->PlayerP.AbsTileX + RelColumn;
      uint32 TileValue = GetTileValue(TileMap, Column, Row, GameState->PlayerP.AbsTileZ);
      if(TileValue > 0)
      {
        if(TileValue == 2)
        {
          gray = 1.0;
        }
        if(Row == GameState->PlayerP.AbsTileY && Column == GameState->PlayerP.AbsTileX)
        {
          gray = 0.0;
        }
        if(TileValue == 3)
        {
          gray = 0.25;
        }
        real32 TileCenX = ScreenCenterX + (RelColumn * TileSideInPixels) - MetersToPixels * GameState->PlayerP.TileRelX;
        real32 TileCenY = ScreenCenterY - (RelRow * TileSideInPixels) + MetersToPixels * GameState->PlayerP.TileRelY;
        real32 MinX = TileCenX - 0.5f * TileSideInPixels;
        real32 MinY = TileCenY - 0.5f * TileSideInPixels;
        real32 MaxX = MinX + TileSideInPixels;
        real32 MaxY = MinY + TileSideInPixels;

        DrawRect(Buffer,
          MinX, MaxX,
          MinY, MaxY,
          gray, gray, gray);
      }
    }
  }
  real32 PlayerLeft = ScreenCenterX - 0.5f * MetersToPixels * PlayerWidth;
  real32 PlayerTop = ScreenCenterY - 0.5f * MetersToPixels * PlayerHeight;
  DrawRect(Buffer,
    PlayerLeft,
    PlayerLeft + PlayerWidth * MetersToPixels,
    PlayerTop,
    PlayerTop + PlayerHeight * MetersToPixels,
    0.7f, 0.8f, 0.9f);
}

extern "C" GAME_UPDATE_AUDIO(GameUpdateAudio)
{
  game_state *GameState = (game_state *)Memory->PermanentStorage;
  UpdateSound(SoundBuffer, 40);
}
