#include "handmade.h"

internal void
RenderWeirdGradeint(game_offscreen_buffer *Buffer, int XOffset, int YOffset) {
  uint8 *Row = (uint8 *)Buffer->Memory;

  for(int Y = 0; Y < Buffer->Height; Y++) {
    uint32 *Pixel = (uint32 *)Row;

    for(int X = 0; X < Buffer->Width; X++) {
      uint8 Blue = (uint8)(X + XOffset);
      uint8 Green = (uint8)(Y + YOffset);
      // 0xXXRRGGBB
      *Pixel++ = (Green << 8) | Blue;
    }

    Row = Row + Buffer->Width * Buffer->BytesPerPixel;
  }
}

internal void
RenderSineWave(game_sound_buffer *Buffer, int ToneHz) {
  local_persist real32 tSine;
  int WavePeriod = Buffer->SamplesPerSecond / ToneHz;

  int16* Output = Buffer->Memory;
  for(int SampleIndex = 0; SampleIndex < Buffer->SampleCount; SampleIndex++) {
    int16 Value = (int16)(sinf(tSine) * Buffer->ToneVolume);
    *Output++ = Value;
    *Output++ = Value;
    tSine += (real32)(2.0f * Pi32 * (1.0f / (real32)WavePeriod));
  }
}

void
GameUpdateAndRender(
  game_memory *Memory,
  game_input *Input,
  game_offscreen_buffer *Buffer,
  game_sound_buffer *SoundBuffer
) {
  game_state *State = (game_state *)Memory->PermanentStorage;

  Assert(sizeof(game_state) <= Memory->PermanentStorageSize)
  if(!Memory->IsInitialized) {
    State->ToneHz = 256;
    Memory->IsInitialized = true;
  }

  game_controller_input *Controller = &Input->Controllers[0];
  if(Controller->Up.IsEndedDown) {
    State->YOffset += 10;
  }
  if(Controller->Down.IsEndedDown) {
    State->YOffset -= 10;
  }
  if(Controller->Left.IsEndedDown) {
    State->XOffset -= 10;
  }
  if(Controller->Right.IsEndedDown) {
    State->XOffset += 10;
  }
  if(State->YOffset <= 1000 && State->YOffset >= -1000) {
    int Tmp = (int)(((real32)State->YOffset / 1000) * 256) + 512;
    State->ToneHz = Tmp;
  }

  RenderWeirdGradeint(Buffer, State->XOffset, State->YOffset);
  RenderSineWave(SoundBuffer, State->ToneHz);
}
