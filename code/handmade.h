#ifndef HANDMADE_H__
#define HANDMADE_H__

struct game_offscreen_buffer
{
  void *Memory;
  int Width;
  int Height;
  int Pitch;
};

struct game_sound_buffer
{
  int SampleCount;
  int16_t *Samples;
  int WavePeriod;
};

internal void
GameUpdateAndRender(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset, game_sound_buffer *SoundBuffer);

#endif
