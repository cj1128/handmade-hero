/*
* @Author: dingxijin
* @Date:   2015-04-21 08:09:18
* @Last Modified by:   dingxijin
* @Last Modified time: 2015-05-13 17:51:34
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
    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;
    tSine += 2.0f * Pi32 / (float)(WavePeriod);
    if( tSine > 2.0f * Pi32 )
    {
      tSine -= 2.0f * Pi32;
    }
  }
}

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

      *Pixel++ = ((Green << 8) | Blue );

    }
    Row += Buffer->Pitch;
  }
}

GAME_UPDATE_VIDEO(GameUpateVideo)
{

  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
  game_state *GameState = (game_state *)Memory->PermanentStorage;
  if(!Memory->IsInitialized)
  {
    char *Filename = __FILE__;
    debug_read_file_result File = Memory->DEBUGPlatformReadFile(Filename);
   if(File.Content)
    {
      Memory->DEBUGPlatformWriteFile("test.txt", File.ContentSize, File.Content);
      Memory->DEBUGPlatformFreeFileMemory(File.Content);
    }
    GameState->ToneHz = 256;
    Memory->IsInitialized = true;
  }

  for(int ControllerIndex = 0 ;
    ControllerIndex < ArrayCount(Input->Controllers);
    ControllerIndex++)
  {
    game_controller_input *Controller = GetController(Input, ControllerIndex);
    if(Controller->IsAnalog)
    {
      GameState->BlueOffset += (int)(4.0f * Controller->StickAverageX);
      GameState->GreenOffset = 256 + (int)(128.0f * Controller->StickAverageY);
    }
    else
    {
      if(Controller->MoveLeft.EndedDown)
      {
        GameState->BlueOffset -= 10;
      }
      else if(Controller->MoveRight.EndedDown)
      {
       GameState->BlueOffset += 10;
      }
      else if(Controller->MoveUp.EndedDown)
      {
       GameState->GreenOffset += 10;
      }
      else if(Controller->MoveDown.EndedDown)
      {
       GameState->GreenOffset -= 10;
      }
    }
  }

  RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}

GAME_UPDATE_AUDIO(GameUpdateAudio)
{
  game_state *GameState = (game_state *)Memory->PermanentStorage;
  UpdateSound(SoundBuffer, GameState->ToneHz);
}
