#ifndef WIN32_HANDMADE_H__
#define WIN32_HANDMADE_H__

struct win32_offscreen_buffer{
    //pixel memroy order: bb gg rr xx
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int BytesPerPixel;
    int Pitch;
};

struct win32_window_dimension
{
    int Width;
    int Height;
};


struct win32_sound_output
{
    int SamplesPerSecond;
    uint32 RunningSampleIndex;
    int WavePeriod;
    int BytesPerSample;
    uint32 SecondaryBufferSize;
    float  tSine;
    int LatencySampleCount;
    int SafetyBytes;
};

struct win32_debug_time_marker
{
    DWORD OutputPlayCursor;
    DWORD OutputWriteCursor;
    DWORD OutputLocation;
    DWORD OutputByteCount;
    DWORD ExpectedFlipPlayCursor;

    DWORD FlipPlayCursor;
    DWORD FlipWriteCursor;
};

#endif