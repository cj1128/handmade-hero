#include "handmade.h"

internal void
RenderWeirdGradeint(game_offscreen_buffer *Buffer, int XOffset, int YOffset) {
  uint8 *Row = (uint8 *)Buffer->Memory;

  for(int Y = 0; Y < Buffer->Height; Y++) {
    uint32 *Pixel = (uint32 *)Row;

    for(int X = 0; X < Buffer->Width; X++) {
      uint8 Blue = X + XOffset;
      uint8 Green = Y + YOffset;
      // 0xXXRRGGBB
      *Pixel++ = (Green << 8) | Blue;
    }

    Row = Row + Buffer->Width * Buffer->BytesPerPixel;
  }
}

internal void
RenderSineWave(game_sound_buffer *Buffer) {
  local_persist real32 tSine;
  int WavePeriod = Buffer->SamplesPerSecond / Buffer->ToneHz;

  int16* Output = Buffer->Memory;
  for(int SampleIndex = 0; SampleIndex < Buffer->SampleCount; SampleIndex++) {
    int16 Value = (int16)(sinf(tSine) * Buffer->ToneVolume);
    *Output++ = Value;
    *Output++ = Value;
    tSine += 2.0f * Pi32 * (1.0f / (real32)WavePeriod);
  }
}

internal void
GameUpdateAndRender(
  game_offscreen_buffer *Buffer,
  game_sound_buffer *SoundBuffer,
  int XOffset, int YOffset
) {
  RenderWeirdGradeint(Buffer, XOffset, YOffset);
  RenderSineWave(SoundBuffer);
}
