#include <stdint.h>
#include <math.h>

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

#define Pi32 3.14159265359
#define global_variable static
#define local_persist static
#define internal static

#include "handmade.cpp"

#include <windows.h>
#include <malloc.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>

struct win32_offscreen_buffer {
  void *Memory;
  BITMAPINFO Info;
  int BytesPerPixel;
  int Width;
  int Height;
};

struct win32_window_dimension {
  int Width;
  int Height;
};

struct win32_sound_output {
      int32 SamplesPerSecond;
      int32 BytesPerSample;
      int32 BufferSize;
      int32 LatencySampleCount;
      int ToneHz;
      int16 ToneVolume;
      uint32 RunningSampleIndex;
};

// XInputGetState
#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef XINPUT_GET_STATE(x_input_get_state);
XINPUT_GET_STATE(XInputGetStateStub) {
  return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;

// XInputSetState
#define XINPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef XINPUT_SET_STATE(x_input_set_state);
XINPUT_SET_STATE(XInputSetStateStub) {
  return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

global_variable bool GlobalRunning = true;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable int GlobalXOffset;
global_variable int GlobalYOffset;
global_variable LPDIRECTSOUNDBUFFER GlobalSoundBuffer;

internal void
Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize) {
  HMODULE Library = LoadLibraryA("dsound.dll");
  if(Library) {
    direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(Library, "DirectSoundCreate");

    LPDIRECTSOUND DirectSound;

    if(SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) {
      if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))) {
        OutputDebugStringA("Set cooperative level ok\n");
      } else {
        // TODO: logging
      }

      WAVEFORMATEX WaveFormat  = {};
      WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
      WaveFormat.nChannels = 2;
      WaveFormat.nSamplesPerSec = SamplesPerSecond;
      WaveFormat.wBitsPerSample = 16;
      WaveFormat.nBlockAlign = WaveFormat.nChannels * WaveFormat.wBitsPerSample / 8;
      WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;

      {
        DSBUFFERDESC BufferDesc = {};
        BufferDesc.dwSize = sizeof(BufferDesc);
        BufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
        LPDIRECTSOUNDBUFFER  PrimaryBuffer;
        if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDesc, &PrimaryBuffer, 0))) {
            OutputDebugStringA("Create primary buffer ok\n");
            if(SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat))) {
              OutputDebugStringA("Primary buffer set format ok\n");
            } else {
              // TDOO: logging
            }
        }
      }

      {
        DSBUFFERDESC BufferDesc = {};
        BufferDesc.dwSize = sizeof(BufferDesc);
        BufferDesc.dwFlags = 0;
        BufferDesc.dwBufferBytes = BufferSize;
        BufferDesc.lpwfxFormat = &WaveFormat;
        if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDesc, &GlobalSoundBuffer, 0))) {
          OutputDebugStringA("Secondary buffer created\n");
        } else {
          // TODO: logging
        }
      }

    } else {
      // TODO: logging
    }
  } else {
    // TODO: logging
  }
}

internal void
Win32ClearSoundBuffer(win32_sound_output *SoundOutput) {
  void *Region1;
  DWORD Region1Size;
  void *Region2;
  DWORD Region2Size;

  if(SUCCEEDED(GlobalSoundBuffer->Lock(
    0,
    SoundOutput->BufferSize,
    &Region1, &Region1Size,
    &Region2, &Region2Size,
    0
  ))) {
    uint8 *Out = (uint8 *)Region1;

    for(int ByteIndex = 0; ByteIndex < Region1Size; ByteIndex++) {
      *Out++ = 0;
    }

    Out = (uint8 *)Region2;

    for(int ByteIndex = 0; ByteIndex < Region2Size; ByteIndex++) {
      *Out++ = 0;
    }

    GlobalSoundBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
  }
}

internal void
Win32FillSoundBuffer(
  win32_sound_output *SoundOutput,
  game_sound_buffer *SourceBuffer,
  DWORD LockOffset, DWORD BytesToLock
) {
  void *Region1;
  DWORD Region1Size;
  void *Region2;
  DWORD Region2Size;

  if(SUCCEEDED(GlobalSoundBuffer->Lock(
    LockOffset,
    BytesToLock,
    &Region1, &Region1Size,
    &Region2, &Region2Size,
    0
  ))) {
    int16 *Dest = (int16 *)Region1;
    int16 *Source = SourceBuffer->Memory;

    for(int SampleIndex = 0; SampleIndex < Region1Size / SoundOutput->BytesPerSample; SampleIndex++) {
      *Dest++ = *Source++;
      *Dest++ = *Source++;
      SoundOutput->RunningSampleIndex++;
    }

    Dest = (int16 *)Region2;
    for(int SampleIndex = 0; SampleIndex < Region2Size / SoundOutput->BytesPerSample; SampleIndex++) {
      *Dest++ = *Source++;
      *Dest++ = *Source++;
      SoundOutput->RunningSampleIndex++;
    }

    GlobalSoundBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
  }
}

internal void
Win32LoadXInput() {
  HMODULE Library = LoadLibraryA("xinput1_4.dll");
  if(Library) {
    XInputGetState = (x_input_get_state *)GetProcAddress(Library, "XInputGetState");
    XInputSetState = (x_input_set_state *)GetProcAddress(Library, "XInputSetState");
  }
}

internal win32_window_dimension
Win32GetWindowDimension(HWND Window) {
  win32_window_dimension Result;
  RECT ClientRect;
  GetClientRect(Window, &ClientRect);
  Result.Width = ClientRect.right - ClientRect.left;
  Result.Height = ClientRect.bottom - ClientRect.top;
  return Result;
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height) {
  if(Buffer->Memory) {
    VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
  }

  Buffer->Width = Width;
  Buffer->Height = Height;
  Buffer->BytesPerPixel = 4;

  Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
  Buffer->Info.bmiHeader.biWidth = Buffer->Width;
  Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
  Buffer->Info.bmiHeader.biPlanes = 1;
  Buffer->Info.bmiHeader.biBitCount = Buffer->BytesPerPixel * 8;
  Buffer->Info.bmiHeader.biCompression = BI_RGB;

  int BitmapSize = Buffer->Width * Buffer->Height * Buffer->BytesPerPixel;

  Buffer->Memory = VirtualAlloc(0, BitmapSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
}

internal void
Win32UpdateWindow(
  HDC DeviceContext,
  int WindowWidth,
  int WindowHeight,
  win32_offscreen_buffer *Buffer
  ) {
  StretchDIBits(
    DeviceContext,
    0, 0, WindowWidth, WindowHeight, // dest
    0, 0, Buffer->Width, Buffer->Height, // src
    Buffer->Memory,
    &Buffer->Info,
    DIB_RGB_COLORS,
    SRCCOPY
    );
}

internal LRESULT CALLBACK
Win32MainWindowCallback(
  HWND Window,
  UINT Message,
  WPARAM WParam,
  LPARAM LParam
  ) {
  LRESULT Result = 0;
  switch(Message) {
    case WM_ACTIVATEAPP: {
      OutputDebugStringA("WM_ACTIVATEAPP\n");
    } break;

    case WM_CLOSE: {
      GlobalRunning = false;
      OutputDebugStringA("WM_CLOSE\n");
    } break;

    case WM_DESTROY: {
      OutputDebugStringA("WM_DESTROY\n");
    } break;

    case WM_SYSKEYUP:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_KEYDOWN: {
      uint32 VKCode = WParam;
      bool32 WasDown = LParam & (1 << 30);
      bool32 IsDown = LParam & (1 << 31);

      int diff = 20;

      switch(VKCode) {
        case VK_F4: {
          bool32 IsAltDown = LParam & (1 << 29);
          if(IsAltDown) {
            GlobalRunning = false;
          }
        } break;

        case 'W':
        case VK_UP: {
          GlobalYOffset += diff;
        } break;

        case 'S':
        case VK_DOWN: {
          GlobalYOffset -= diff;
        } break;

        case 'A':
        case VK_LEFT: {
          GlobalXOffset -= diff;
        } break;

        case 'D':
        case VK_RIGHT: {
          GlobalXOffset += diff;
        } break;
      }

    } break;

    case WM_PAINT:
    {
      PAINTSTRUCT PaintStruct = {};
      HDC DeviceContext = BeginPaint(Window, &PaintStruct);
      win32_window_dimension Dimension = Win32GetWindowDimension(Window);
      Win32UpdateWindow(
        DeviceContext,
        Dimension.Width,
        Dimension.Height,
        &GlobalBackBuffer
        );
      EndPaint(Window, &PaintStruct);
    } break;

    case WM_SIZE:
    {
      OutputDebugStringA("WM_SIZE\n");
    } break;

    default:
    {
      Result = DefWindowProcA(Window, Message, WParam, LParam);
    } break;
  }

  return Result;
}

int CALLBACK
WinMain(
  HINSTANCE Instance,
  HINSTANCE PrevInstance,
  LPSTR     CmdLine,
  int       ShowCmd
  ) {
  LARGE_INTEGER CounterPerSecond;
  QueryPerformanceFrequency(&CounterPerSecond);
  Win32LoadXInput();

  WNDCLASS WindowClass = {};

  // TODO: Check if we need these
  WindowClass.style = CS_HREDRAW | CS_VREDRAW;

  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = Instance;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";

  if(RegisterClass(&WindowClass)) {
    HWND Window = CreateWindowEx(
      0,
      "HandmadeHeroWindowClass",
      "Handmade Hero",
      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      0,
      0,
      Instance,
      0
      );

    if(Window) {
      win32_sound_output SoundOutput = {};
      SoundOutput.SamplesPerSecond = 48000;
      SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 20;
      SoundOutput.BytesPerSample = 4;
      SoundOutput.BufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
      SoundOutput.ToneHz = 256;
      SoundOutput.ToneVolume = 3000;
      SoundOutput.RunningSampleIndex = 0;

      int16 *SoundMemory = (int16 *)VirtualAlloc(0, SoundOutput.BufferSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);

      Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.BufferSize);
      Win32ClearSoundBuffer(&SoundOutput);
      GlobalSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);

      Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

      LARGE_INTEGER LastCounter;
      uint64 LastCycleCounter = __rdtsc();
      QueryPerformanceCounter(&LastCounter);
      while(GlobalRunning) {
        MSG Message = {};

        while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
          TranslateMessage(&Message);
          DispatchMessage(&Message);
        }

        for(int i=0; i< XUSER_MAX_COUNT; i++) {
          XINPUT_STATE state;
          ZeroMemory(&state, sizeof(XINPUT_STATE));
          if(XInputGetState(i, &state) == ERROR_SUCCESS ) {
            XINPUT_GAMEPAD *Pad = &state.Gamepad;

            bool32 Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
            bool32 Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
            bool32 Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
            bool32 Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
            bool32 Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
            bool32 Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
            bool32 LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
            bool32 RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
            bool32 AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
            bool32 BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
            bool32 XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
            bool32 YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

            int16 StickX = Pad->sThumbLX;
            int16 StickY = Pad->sThumbLY;
          }
          else{
            // Controller is not connected
          }
        }

        DWORD BytesToLock;
        DWORD LockOffset;
        bool32 SoundIsValid = false;
        {
          DWORD PlayCursor;
          DWORD WriteCursor;
          if(SUCCEEDED(GlobalSoundBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor))) {
            LockOffset = SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample % SoundOutput.BufferSize;
            DWORD TargetCursor = (PlayCursor + SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample) % SoundOutput.BufferSize;

            if(LockOffset > TargetCursor) {
              BytesToLock = SoundOutput.BufferSize - LockOffset + TargetCursor;
            } else {
              BytesToLock = TargetCursor - LockOffset;
            }

            SoundIsValid = true;
          }
        }

        game_offscreen_buffer Buffer = {};
        Buffer.Memory = GlobalBackBuffer.Memory;
        Buffer.BytesPerPixel = GlobalBackBuffer.BytesPerPixel;
        Buffer.Width = GlobalBackBuffer.Width;
        Buffer.Height = GlobalBackBuffer.Height;

        game_sound_buffer SoundBuffer = {};
        SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
        SoundBuffer.SampleCount = BytesToLock / SoundOutput.BytesPerSample;
        SoundBuffer.ToneHz = SoundOutput.ToneHz;
        SoundBuffer.ToneVolume = SoundOutput.ToneVolume;
        SoundBuffer.Memory = SoundMemory;

        GameUpdateAndRender(&Buffer, &SoundBuffer, GlobalXOffset, GlobalYOffset);

        if(GlobalYOffset <= 1000 && GlobalYOffset >= -1000) {
          int Tmp = (int)(((real32)GlobalYOffset / 1000) * 256) + 512;
          SoundOutput.ToneHz = Tmp;
        }

        if(SoundIsValid) {
          Win32FillSoundBuffer(&SoundOutput, &SoundBuffer, LockOffset, BytesToLock);
        }

        HDC DeviceContext = GetDC(Window);
        win32_window_dimension Dimension = Win32GetWindowDimension(Window);
        Win32UpdateWindow(
          DeviceContext,
          Dimension.Width,
          Dimension.Height,
          &GlobalBackBuffer
          );
        ReleaseDC(Window, DeviceContext);

        uint64 CurrentCycleCounter = __rdtsc();
        LARGE_INTEGER CurrentCounter;
        QueryPerformanceCounter(&CurrentCounter);

        int64 CounterElapsed = CurrentCounter.QuadPart - LastCounter.QuadPart;
        uint64 CycleElapsed = CurrentCycleCounter - LastCycleCounter;

        real32 MSPerFrame = 1000.0f * (real32)CounterElapsed / (real32)CounterPerSecond.QuadPart;
        real32 FPS = (real32)CounterPerSecond.QuadPart / (real32)CounterElapsed;
        real32 MCPF = (real32)CycleElapsed / (1000.0f * 1000.0f);

        char OutputBuffer[256];
        sprintf(OutputBuffer, "ms/f: %.2f,  fps: %.2f,  mc/f: %.2f\n", MSPerFrame, FPS, MCPF);
        OutputDebugStringA(OutputBuffer);

        LastCounter = CurrentCounter;
        LastCycleCounter = CurrentCycleCounter;
      }
    } else {
      // TODO: Logging
    }
  } else {
    // TDOO: Logging
  }

  return 0;
}
