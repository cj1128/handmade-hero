#if !defined(HANDMADE_H)

struct game_offscreen_buffer {
  void *Memory;
  int BytesPerPixel;
  int Width;
  int Height;
};

struct game_sound_buffer {
  int SamplesPerSecond;
  int SampleCount;
  int ToneHz;
  int ToneVolume;
  int16 *Memory;
};

void GameUpdateAndRender(
  game_offscreen_buffer* Buffer,
  game_sound_buffer* SoundBuffer,
  int XOffset,
  int YOffset
);

#define HANDMADE_H
#endif
