#ifndef HANDMADE_H

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

struct game_offscreen_buffer {
  void *Memory;
  int BytesPerPixel;
  int Width;
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
  bool32 IsAnalog;

  real32 StartX;
  real32 EndX;
  real32 MinX;
  real32 MaxX;

  real32 StartY;
  real32 EndY;
  real32 MinY;
  real32 MaxY;

  union {
    game_button_state Buttons[6];
    struct {
      game_button_state A;
      game_button_state B;
      game_button_state X;
      game_button_state Y;
      game_button_state LeftShoulder;
      game_button_state RightShoulder;
    };
  };
};

struct game_input {
  game_controller_input Controllers[4];
};

struct game_memory {
  bool32 IsInitialized;

  uint64 PermanentStorageSize;
  void *PermanentStorage; // required to be cleared to zero

  uint64 TransientStorageSize;
  void *TransientStorage;
};

void GameUpdateAndRender(
  game_memory *Memory,
  game_input *Input,
  game_offscreen_buffer* Buffer,
  game_sound_buffer* SoundBuffer
);

//
// not needed by platform layer
//

struct game_state {
  int XOffset;
  int YOffset;
  int ToneHz;
};

#define HANDMADE_H
#endif
