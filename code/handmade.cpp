/*
* @Author: dingxijin
* @Date:   2015-04-21 08:09:18
* @Last Modified by:   dingxijin
* @Last Modified time: 2015-04-25 20:56:32
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
      uint8_t Blue = X + BlueOffset;
      uint8_t Green = Y + GreenOffset;

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
    GameState->ToneHz = 256;
    GameState->BlueOffset = 0;
    GameState->GreenOffset = 0;
  }

  game_controller_input *controller1 = &Input->Controllers[0];
  if(controller1->IsAnalog)
  {
    //TODO: do some stuff
  }
  else
  {
    //TODO: do some stuff
  }

  if(controller1->Down.EndedDown)
  {
    //TODO: do some stuff
  }

  UpdateSound(SoundBuffer, ToneHz);
  RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}