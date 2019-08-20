#if !defined(HANDMADE_H)

struct game_offscreen_buffer {
  void *Memory;
  int BytesPerPixel;
  int Width;
  int Height;
};

#define HANDMADE_H
#endif
