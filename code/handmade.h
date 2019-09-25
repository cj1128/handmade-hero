#ifndef HANDMADE_H

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
debug_read_file_result DebugPlatformReadFile(char *FileName);
bool32 DebugPlatformWriteFile(char *FileName, void *Memory, uint32 Size);
void DebugPlatformFreeFileMemory(void *Memory);
#endif

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
};

void GameUpdateVideo(
  game_memory *Memory,
  game_input *Input,
  game_offscreen_buffer* Buffer
);

void GameUpdateAudio(
  game_memory *Memory,
  game_sound_buffer* SoundBuffer
);

//
// not needed by platform layer
//

struct game_state {
  int XOffset;
  int YOffset;
  int ToneHz;
  real32 tSine;
};

#define HANDMADE_H
#endif
