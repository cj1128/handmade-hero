#ifndef HANDMADE_PLATFORM_H


#ifdef __cplusplus
extern "C" {
#endif


#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#define COMPILER_MSVC 1
#else
#define COMPILER_LLVM 1
#endif
#endif

#if COMPILER_MSVC
#include <intrin.h>
#endif

// Types
#include <stdint.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;


struct thread_context
{
    int PlaceHolder;
};


#if HANDMADE_INTERNAL
/*
  NOTE: These are NOT for doing anything for the shipping game
  they are blocking and the write doesn't protect against lost data
*/
struct debug_read_file_result
{
    uint32 ContentSize;
    void *Content;
};


#define DEBUG_PLATFORM_READ_FILE(name) debug_read_file_result name(thread_context *Thread, char *Filename)
typedef DEBUG_PLATFORM_READ_FILE(debug_platform_read_file);

#define DEBUG_PLATFORM_WRITE_FILE(name) bool32 name(thread_context *Thread, char *Filename, uint32 Size, void *Content)
typedef DEBUG_PLATFORM_WRITE_FILE(debug_platform_write_file);

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context *Thread, void *Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#endif


struct game_offscreen_buffer
{
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct game_sound_buffer
{
    int SampleCount;
    int16 *Samples;
    int SamplesPerSecond;
};

struct game_button_state
{
    int HalfTransitionCount;
    bool32 EndedDown;
};

struct game_controller_input
{
    bool32 IsAnalog;
    bool32 IsConnected;
    real32 StickAverageX;
    real32 StickAverageY;

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
    real32 TimeForFrame;
    game_button_state MouseButtons[5];
    int MouseX, MouseY, MouseZ;
    game_controller_input Controllers[5];
};


//NOTE: this memory should be initialized to zero by platform!
struct game_memory
{
    bool32 IsInitialized;
    uint64 PermanentStorageSize;
    void *PermanentStorage;

    uint64 TransientStorageSize;
    void *TransientStorage;

    debug_platform_read_file *DEBUGPlatformReadFile;
    debug_platform_write_file *DEBUGPlatformWriteFile;
    debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;

};


#define GAME_UPDATE_VIDEO(name) void name(thread_context *Thread, \
    game_memory *Memory, \
    game_input *Input, \
    game_offscreen_buffer *Buffer)

typedef GAME_UPDATE_VIDEO(game_update_video);

#define GAME_UPDATE_AUDIO(name) void name(thread_context *Thread, \
    game_memory *Memory, \
    game_sound_buffer *SoundBuffer)

typedef GAME_UPDATE_AUDIO(game_update_audio);

#ifdef __cplusplus
}
#endif


#define HANDMADE_PLATFORM_H
#endif
