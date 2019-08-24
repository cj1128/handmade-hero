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

void GameUpdateAndRender(
  game_input *Input,
  game_offscreen_buffer* Buffer,
  game_sound_buffer* SoundBuffer
);

#define HANDMADE_H
#endif
