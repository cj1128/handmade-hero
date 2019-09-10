#ifndef WIN32_HANDMADE_H
struct win32_offscreen_buffer
{
  // Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
  BITMAPINFO Info;
  void *Memory;
  int Width;
  int Height;
  int Pitch;
  int BytesPerPixel;
};

struct win32_window_dimension
{
  int Width;
  int Height;
};

struct win32_sound_output {
  int32 SamplesPerSecond;
  int32 BytesPerSample;
  int32 BufferSize;
  int32 LatencySampleCount;
  int ToneHz;
  int16 ToneVolume;
  uint32 RunningSampleIndex;
};

#define WIN32_HANDMADE_H
#endif
