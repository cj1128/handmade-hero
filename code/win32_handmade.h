#ifndef WIN32_HANDMADE_H

struct win32_offscreen_buffer
{
  // Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX, 0xXXRRGGBB
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
  DWORD BufferSize;
  int32 SafetyBytes;
  int16 ToneVolume;
  uint32 RunningSampleIndex;
};

struct win32_debug_sound_marker {
  DWORD FlipPlayCursor;
  DWORD FlipWriteCursor;
  DWORD OutputPlayCursor;
  DWORD OutputWriteCursor;
  DWORD LockOffset;
  DWORD BytesToLock;
  DWORD ExpectedFlipPlayCursor;
};

#define WIN32_HANDMADE_H
#endif
