#ifndef WIN32_HANDMADE_H__
#define WIN32_HANDMADE_H__

struct win32_offscreen_buffer{
    //pixel memroy order: bb gg rr xx
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
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
    uint32_t RunningSampleIndex;
    int WavePeriod;
    int BytesPerSample;
    int SecondaryBufferSize;
    float  tSine;
    int LatencySampleCount;
};

#endif