/*
* @Author: dingxijin
* @Date:   2015-04-21 08:09:18
* @Last Modified by:   dingxijin
* @Last Modified time: 2015-06-25 00:51:33
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
    for(int X = 0;
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
  }

  for(int ControllerIndex = 0 ;
    ControllerIndex < ArrayCount(Input->Controllers);
    ControllerIndex++)
  {
    game_controller_input *Controller = GetController(Input, ControllerIndex);
    if(Controller->IsAnalog)
    {
    }
    else
    {
      int dx = 0;
      int dy = 0;
      if(Controller->MoveLeft.EndedDown)
      {
        dx = -100;
      }
      else if(Controller->MoveRight.EndedDown)
      {
        dx = 100;
      }
      else if(Controller->MoveUp.EndedDown)
      {
        dy = -60;
      }
      else if(Controller->MoveDown.EndedDown)
      {
        dy = 60;
      }

      GameState->PlayerX += Input->TimeForFrame * dx;
      GameState->PlayerY += Input->TimeForFrame * dy;
    }
  }

  uint8 TileMap[10][10] =
  {
    {1, 1, 1, 1,  1, 1, 0, 0,  0, 0},
    {0, 1, 0, 0,  0, 1, 0, 0,  0, 0},
    {0, 0, 0, 0,  0, 1, 0, 0,  0, 0},
    {0, 0, 0, 0,  0, 0, 0, 0,  0, 0},
    {0, 0, 0, 0,  0, 0, 0, 0,  0, 0},
    {0, 0, 0, 0,  0, 0, 0, 0,  0, 0},
    {0, 0, 0, 0,  0, 0, 1, 1,  0, 0},
    {0, 0, 0, 0,  0, 1, 0, 1,  0, 0},
    {0, 0, 0, 0,  0, 1, 0, 1,  0, 0},
    {0, 0, 0, 0,  0, 1, 0, 1,  0, 0},
  };

  DrawRect(Buffer, 0.0f, (real32)Buffer->Width, 0.0f, (real32)Buffer->Height, 0.7f, 0.9f, 1.0f);
  real32 TileWidth = 50;
  real32 TileHeight = 50;

  for(int y = 0; y < 10; y++)
  {
    for(int x = 0; x < 10; x++)
    {
      real32 gray = 0.5;
      if(TileMap[x][y] == 1)
      {
        gray = 1.0;
      }
      DrawRect(Buffer, x * TileWidth,
        x * TileWidth + TileWidth,
        y * TileHeight,
        y * TileHeight + TileHeight,
        gray, gray, gray
        );
    }
  }
  real32 PX = GameState -> PlayerX;
  real32 PY = GameState -> PlayerY;
  DrawRect(Buffer, PX - TileWidth/2, PX + TileWidth/2, PY - TileHeight, PY, 0.7f, 0.8f, 0.9f);
}

extern "C" GAME_UPDATE_AUDIO(GameUpdateAudio)
{
  game_state *GameState = (game_state *)Memory->PermanentStorage;
  UpdateSound(SoundBuffer, 40);
}
