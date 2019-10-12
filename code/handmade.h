#ifndef HANDMADE_H

#include <stdint.h>
#include <math.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int32 bool32;
typedef float real32;
typedef double real64;

#define Pi32 3.14159265359
#define global_variable static
#define local_persist static
#define internal static

#define ArrayCount(arr) (sizeof((arr)) / (sizeof((arr)[0])))
#define Kilobytes(number) ((number) * 1024ull)
#define Megabytes(number) (Kilobytes(number) * 1024ull)
#define Gigabytes(number) (Megabytes(number) * 1024ull)
#define Terabytes(number) (Gigabytes(number) * 1024ull)

#ifdef HANDMADE_SLOW
#define Assert(expression) if(!(expression)) { *(int*) 0 = 0; }
#else
#define Assert(expression)
#endif

/*
  HANDMADE_INTERNAL:
    0: public build
    1: internal build

  HANDMADE_SLOW:
    1: slow code allowed
    0: no slow code
*/

inline uint32
SafeTruncateUInt64(uint64 Value) {
  Assert(Value <= 0xFFFFFFFF)
  uint32 Result = (uint32)Value;
  return Result;
}

#ifdef HANDMADE_INTERNAL

struct debug_read_file_result {
  uint32 Size;
  void *Memory;
};

#define DEBUG_PLATFORM_READ_FILE(name) debug_read_file_result name(char *FileName)
typedef DEBUG_PLATFORM_READ_FILE(debug_platform_read_file);

#define DEBUG_PLATFORM_WRITE_FILE(name) bool32 name(char *FileName, void *Memory, uint32 FileSize)
typedef DEBUG_PLATFORM_WRITE_FILE(debug_platform_write_file);

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void *Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#endif

struct game_offscreen_buffer {
  void *Memory;
  int BytesPerPixel;
  int Width;
  int Pitch;
  int Height;
};

struct game_sound_buffer {
  int SamplesPerSecond;
  int SampleCount;
  int ToneVolume;
  int16 *Memory;
};

struct game_button_state {
  int32 HalfTransitionCount;
  bool32 IsEndedDown;
};

struct game_controller_input {
  bool32 IsConnected;
  bool32 IsAnalog;

  real32 StickAverageX;
  real32 StickAverageY;

  union {
    game_button_state Buttons[12];
    struct {
      // D-pad
      game_button_state MoveUp;
      game_button_state MoveDown;
      game_button_state MoveLeft;
      game_button_state MoveRight;

      game_button_state ActionUp; // Y
      game_button_state ActionDown; // A
      game_button_state ActionLeft; // X
      game_button_state ActionRight; // B

      game_button_state LeftShoulder;
      game_button_state RightShoulder;

      game_button_state Start;
      game_button_state Back;

      // never goes down
      game_button_state Terminator;
    };
  };
};

struct game_input {
  game_controller_input Controllers[5];
};

struct game_memory {
  bool32 IsInitialized;

  uint64 PermanentStorageSize;
  void *PermanentStorage; // required to be cleared to zero

  uint64 TransientStorageSize;
  void *TransientStorage;

  debug_platform_read_file *DebugPlatformReadFile;
  debug_platform_write_file *DebugPlatformWriteFile;
  debug_platform_free_file_memory *DebugPlatformFreeFileMemory;
};

#define GAME_UPDATE_VIDEO(name) void name(game_memory *Memory, game_input *Input, game_offscreen_buffer* Buffer)
typedef GAME_UPDATE_VIDEO(game_update_video);

#define GAME_UPDATE_AUDIO(name) void name(game_memory *Memory, game_sound_buffer* SoundBuffer)
typedef GAME_UPDATE_AUDIO(game_update_audio);

//
// not needed by platform layer
//

struct game_state {
  int XOffset;
  int YOffset;
  int ToneHz;

  int PlayerX;
  int PlayerY;

  real32 tSine;
  real32 tJump;
};

#define HANDMADE_H
#endif
