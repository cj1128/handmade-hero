#ifndef HANDMADE_H__
#define HANDMADE_H__

#include <math.H>

#define internal static
#define local_persist static
#define global_variable static


//TODO: Implements sinf by myself
#define Pi32 3.14159265359f

#define ArrayCount(Array) (sizeof((Array)) / sizeof((Array)[0]))

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

internal void
GameUpdateAndRender(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset, game_sound_buffer *SoundBuffer);

#endif
