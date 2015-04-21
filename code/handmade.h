#ifndef HANDMADE_H__
#define HANDMADE_H__

struct game_offscreen_buffer
{
  void *Memory;
  int Width;
  int Height;
  int Pitch;
};

internal void
GameUpdateAndRender(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset);

#endif
