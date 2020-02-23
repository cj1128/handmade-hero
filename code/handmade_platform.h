#ifndef HANDMADE_PLATFORM_H
#define HANDMADE_PLATFORM_H

#include <stdint.h>
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

/*
  HANDMADE_INTERNAL:
    0: public build
    1: internal build

  HANDMADE_SLOW:
    1: slow code allowed
    0: no slow code
*/

// Compilers
#if !defined(COMPILER_MSVC)
#if _MSC_VER
#define COMPILER_MSVC 1
#else
#define COMPILER_MSVC 0
#endif
#endif

#if COMPILER_MSVC
#include <intrin.h>
#endif

#define global_variable static
#define local_persist static
#define internal static

#define ArrayCount(arr) (sizeof((arr)) / (sizeof((arr)[0])))
#define Kilobytes(number) ((number) * 1024ull)
#define Megabytes(number) (Kilobytes(number) * 1024ull)
#define Gigabytes(number) (Megabytes(number) * 1024ull)
#define Terabytes(number) (Gigabytes(number) * 1024ull)
#define Minimum(a, b) ((a) < (b) ? (a) : (b))
#define Maximum(a, b) ((a) > (b) ? (a) : (b))

#ifdef HANDMADE_SLOW
#define Assert(expression) if(!(expression)) { *(int*) 0 = 0; }
#else
#define Assert(expression)
#endif

#define InvalidCodePath Assert(!"InvalidCodePath")

inline uint32
SafeTruncateUInt64(uint64 value) {
  Assert(value <= 0xFFFFFFFF)
  uint32 result = (uint32)value;
  return result;
}

struct thread_context {
  int placeholder;
};

struct game_offscreen_buffer {
  // Byte order: BB GG RR AA, 0xAARRGGBB
  void *memory;
  int bytesPerPixel;
  int width;
  int pitch;
  int height;
};

struct game_sound_buffer {
  int samplesPerSecond;
  int sampleCount;
  int toneVolume;
  int16 *memory;
};

struct game_button_state {
  int32 halfTransitionCount;
  bool32 isEndedDown;
};

struct game_controller_input {
  bool32 isConnected;
  bool32 isAnalog;

  real32 stickAverageX;
  real32 stickAverageY;

  union {
    game_button_state buttons[12];
    struct {
      // D-pad
      game_button_state moveUp;
      game_button_state moveDown;
      game_button_state moveLeft;
      game_button_state moveRight;

      game_button_state actionUp; // Y
      game_button_state actionDown; // A
      game_button_state actionLeft; // x
      game_button_state actionRight; // B

      game_button_state leftShoulder;
      game_button_state rightShoulder;

      game_button_state start;
      game_button_state back;

      // never goes down
      game_button_state terminator;
    };
  };
};

struct game_input {
  real32 dt;
  int32 mouseX, mouseY;
  game_button_state mouseButtons[5];

  game_controller_input controllers[5];
};

#ifdef HANDMADE_INTERNAL
struct debug_read_file_result {
  uint32 size;
  void *memory;
};

#define DEBUG_PLATFORM_READ_FILE(name) debug_read_file_result name(thread_context *thread, char *fileName)
typedef DEBUG_PLATFORM_READ_FILE(debug_platform_read_file);

#define DEBUG_PLATFORM_WRITE_FILE(name) bool32 name(thread_context *thread, char *fileName, void *memory, uint32 fileSize)
typedef DEBUG_PLATFORM_WRITE_FILE(debug_platform_write_file);

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context *thread, void *memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);
#endif

struct game_memory {
  bool32 isInitialized;

  size_t permanentStorageSize;
  void *permanentStorage; // required to be cleared to zero

  size_t transientStorageSize;
  void *transientStorage;

  debug_platform_read_file *debugPlatformReadFile;
  debug_platform_write_file *debugPlatformWriteFile;
  debug_platform_free_file_memory *debugPlatformFreeFileMemory;
};

#define GAME_UPDATE_VIDEO(name) void name(thread_context *thread, game_memory *memory, game_input *input, game_offscreen_buffer* buffer)
typedef GAME_UPDATE_VIDEO(game_update_video);

#define GAME_UPDATE_AUDIO(name) void name(thread_context *thread, game_memory *memory, game_sound_buffer* soundBuffer)
typedef GAME_UPDATE_AUDIO(game_update_audio);

#endif
