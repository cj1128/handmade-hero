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
  uint32_t Result = (uint32_t)Value;
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
internal bool DEBUGPlatformWriteFile(char *Filename, uint32_t Size, void *Content);
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
  bool IsConnected;
  float StickAverageX;
  float StickAverageY;

  union
  {
    game_button_state Buttons[12];
    struct
    {
      game_button_state MoveUp;
      game_button_state MoveDown;
      game_button_state MoveLeft;
      game_button_state MoveRight;

      game_button_state ActionUp;
      game_button_state ActionDown;
      game_button_state ActionLeft;
      game_button_state ActionRight;

      game_button_state LeftShoulder;
      game_button_state RightShoulder;

      game_button_state Start;
      game_button_state Back;

      //NOTE: All buttons must be added above terminator
      game_button_state Terminator;
    };
  };
};

struct game_input
{
  game_controller_input Controllers[5];
};

inline game_controller_input *GetController(game_input *Input, unsigned int Index)
{
  Assert(Index < ArrayCount(Input->Controllers));
  game_controller_input *Result = &Input->Controllers[Index];
  return Result;
}

//NOTE: this memory should be initialized to zero by platform!
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
GameUpdateAndRender(game_memory *Memory, game_offscreen_buffer *Buffer, game_sound_buffer *SoundBuffer);

#endif
