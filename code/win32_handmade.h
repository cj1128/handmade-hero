#ifndef WIN32_HANDMADE_H

struct win32_offscreen_buffer
{
  // Pixels are alwasy 32-bits wide, memory Order BB GG RR XX, 0xXXRRGGBB
  BITMAPINFO info;
  void *memory;
  int width;
  int height;
  int pitch;
  int bytesPerPixel;
};

struct win32_window_dimension
{
  int width;
  int height;
};

struct win32_sound_output {
  int32 samplesPerSecond;
  int32 bytesPerSample;
  DWORD bufferSize;
  int32 safetyBytes;
  int16 toneVolume;
  uint32 runningSampleIndex;
};

struct win32_debug_sound_marker {
  DWORD flipPlayCursor;
  DWORD flipWriteCursor;
  DWORD outputPlayCursor;
  DWORD outputWriteCursor;
  DWORD lockOffset;
  DWORD bytesToLock;
  DWORD expectedFlipPlayCursor;
};

struct win32_game_code {
  HMODULE gameDLL;
  FILETIME  gameDLLLastWriteTime;
  bool32 isValid;
  game_update_video *gameUpdateVideo;
  game_update_audio *gameUpdateAudio;
};

#define WIN32_MAX_PATH MAX_PATH

struct win32_replay_buffer {
  HANDLE inputHandle;
  void *memory;
};

struct win32_state {
  int inputRecordingIndex; // 1-based, 0 means no recording
  int inputPlayingBackIndex; // 1-based, 0 means no playing back

  win32_replay_buffer replayBuffers[4];

  void *gameMemory;
  uint64 memorySize;

  char exePath[WIN32_MAX_PATH];
  int exeDirLength;
};

#define WIN32_HANDMADE_H
#endif
