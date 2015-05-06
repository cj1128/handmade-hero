/*
* @Author: dingxijin
* @Date:   2015-04-21 08:09:18
* @Last Modified by:   dingxijin
* @Last Modified time: 2015-05-06 10:26:26
*/

#include "handmade.h"

internal void
UpdateSound(game_sound_buffer *SoundBuffer, int ToneHz)
{
  local_persist float tSine;
  int ToneVolume = 10000;
  int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;
  int16_t *SampleOut = SoundBuffer->Samples;
  for(int SampleIndex = 0 ;SampleIndex < SoundBuffer->SampleCount ;
    SampleIndex++)
  {
    float SineValue = sinf(tSine);
    int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;
    tSine += 2.0f * Pi32 / (float)(WavePeriod);
  }
}

internal void
RenderWeirdGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{
  uint8_t *Row = (uint8_t *)Buffer->Memory;

  for(int Y = 0;Y < Buffer->Height; Y++)
  {
    uint32_t *Pixel = (uint32_t *)Row;
    for(int X = 0; X < Buffer->Width; X++)
    {
      // Memory Order: BB GG RR XX
      // 0xXXRRGGBB
      uint8_t Blue = (uint8_t)(X + BlueOffset);
      uint8_t Green = (uint8_t)(Y + GreenOffset);

      *Pixel++ = ((Green << 8) | Blue );

    }
    Row += Buffer->Pitch;
  }
}

internal void
GameUpdateAndRender(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer, game_sound_buffer *SoundBuffer, int ToneHz)
{

  Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
  game_state *GameState = (game_state *)Memory->PermanentStorage;
  if(!Memory->IsInitialized)
  {
    char *Filename = __FILE__;
    debug_read_file_result File = DEBUGPlatformReadFile(Filename);
   if(File.Content)
    {
      DEBUGPlatformWriteFile("test.txt", File.ContentSize, File.Content);
      DEBUGPlatformFreeFileMemory(File.Content);
    }
    GameState->ToneHz = 256;
  }

  game_controller_input *Controller0 = &Input->Controllers[0];
  if(Controller0->IsAnalog)
  {
    //TODO: do some stuff
    GameState->BlueOffset += (int)(4.0f * Controller0->EndX);
    GameState->GreenOffset = 256 + (int)(128.0f * Controller0->EndY);
  }
  else
  {
    //TODO: do some stuff
  }

  if(Controller0->Down.EndedDown)
  {
    GameState->GreenOffset += 10;
  }

  UpdateSound(SoundBuffer, ToneHz);
  RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}