#ifndef HANDMADE_H__
#define HANDMADE_H__

#include <math.H>

#define internal static
#define local_persist static
#define global_variable static


//TODO: Implements sinf by myself
#define Pi32 3.14159265359f

#define ArrayCount(Array) (sizeof((Array)) / sizeof((Array)[0]))

#if HANDMADE_DEBUG
  #define Assert(expression) if(!(expression)){ *(int *)0 = 0; }
#elif
  #define Assert(expression)
#endif

#define Kilobytes(value) ((value)*1024LL)
#define Megabytes(value) (Kilobytes(value)*1024LL)
#define Gigabytes(value) (Megabytes(value)*1024LL)
#define Terabytes(value) (Gigabytes(value)*1024LL)

internal uint32_t
SafeTruncateUint64(uint64_t Value)
{
  Assert( Value <= 0xffffffff );
  uint32_t Result = Value;
  return Result;
}


#if HANDMADE_INTERNAL
/*
NOTE: These are NOT for doing anything for the shipping game
      they are blocking and the write doesn't protect against lost data
 */
struct debug_read_file_result
{
  uint32_t ContentSize;
  void *Content;
};
internal debug_read_file_result DEBUGPlatformReadFile(char *Filename);
internal bool DEBUGPlatformWriteFile(char *Filename, uint64_t Size, void *Content);
internal void DEBUGPlatformFreeFileMemory(void *Memory);

#endif

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
  int SamplesPerSecond;
};

struct game_button_state
{
  int HalfTransitionCount;
  bool EndedDown;
};

struct game_controller_input
{
  bool IsAnalog;
  float StartX;
  float StartY;

  float EndX;
  float EndY;

  float MinX;
  float MinY;

  float MaxX;
  float MaxY;

  union
  {
    game_button_state Buttons[6];
    struct
    {
      game_button_state Up;
      game_button_state Down;
      game_button_state Left;
      game_button_state Right;
      game_button_state LeftShoulder;
      game_button_state RightShoulder;
    };
  };
};

struct game_input
{
  game_controller_input Controllers[4];
};

struct game_memory
{
  bool IsInitialized;
  uint64_t PermanentStorageSize;
  void *PermanentStorage;

  uint64_t TransientStorageSize;
  void *TransientStorage;
};

struct game_state
{
  int ToneHz;
  int BlueOffset;
  int GreenOffset;
};

internal void
GameUpdateAndRender(game_memory *Memory, game_offscreen_buffer *Buffer, game_sound_buffer *SoundBuffer, int ToneHz);

#endif
