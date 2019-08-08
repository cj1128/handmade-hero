#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int32 bool32;

#define global_variable static
#define internal static

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
RenderWeirdGradeint(win32_offscreen_buffer *Buffer, int XOffset, int YOffset) {
  uint8 *Row = (uint8 *)Buffer->Memory;

  for(int Y = 0; Y < Buffer->Height; Y++) {
    uint32 *Pixel = (uint32 *)Row;

    for(int X = 0; X < Buffer->Width; X++) {
      uint8 Blue = X + XOffset;
      uint8 Green = Y + YOffset;
      // 0xXXRRGGBB
      *Pixel++ = (Green << 8) | Blue;
    }

    Row = Row + Buffer->Width * Buffer->BytesPerPixel;
  }
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

        case VK_UP: {
          GlobalYOffset += diff;
        } break;

        case VK_DOWN: {
          GlobalYOffset -= diff;
        } break;

        case VK_LEFT: {
          GlobalXOffset -= diff;
        } break;

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
      Result = DefWindowProc(Window, Message, WParam, LParam);
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
      int32 SamplesPerSecond = 48000;
      int32 BytesPerSample = 4;
      int32 SoundBufferSize = SamplesPerSecond * BytesPerSample;
      int ToneHz = 256;
      int16 ToneVolume = 3000;
      int WavePeriod = SamplesPerSecond / ToneHz;
      int32 HalfWavePeriod = WavePeriod / 2;
      uint32 RunningSampleIndex = 0;

      Win32InitDSound(Window, SamplesPerSecond, SoundBufferSize);
      GlobalSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);
      Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

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

        RenderWeirdGradeint(&GlobalBackBuffer, GlobalXOffset, GlobalYOffset);

        DWORD PlayCursor;
        DWORD WriteCursor;
        if(SUCCEEDED(GlobalSoundBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor))) {
          DWORD LockOffset = RunningSampleIndex * BytesPerSample % SoundBufferSize;
          void *Region1;
          DWORD Region1Size;
          void *Region2;
          DWORD Region2Size;
          DWORD BytesToLock;

          if(PlayCursor == LockOffset) {
            BytesToLock = SoundBufferSize;
          } else if(LockOffset > PlayCursor) {
            BytesToLock = SoundBufferSize - LockOffset + PlayCursor;
          } else {
            BytesToLock = PlayCursor - LockOffset;
          }

          if(SUCCEEDED(GlobalSoundBuffer->Lock(
            LockOffset,
            BytesToLock,
            &Region1, &Region1Size,
            &Region2, &Region2Size,
            0
          ))) {
            int16 *SampleOut = (int16 *)Region1;
            for(int i = 0; i < Region1Size / BytesPerSample; i++) {
              int16 Value = ((RunningSampleIndex++ / HalfWavePeriod) % 2) ? ToneVolume : -ToneVolume;
              *SampleOut++ = Value;
              *SampleOut++ = Value;
            }

            SampleOut = (int16 *)Region2;
            for(int i = 0; i < Region2Size / BytesPerSample; i++) {
              int16 Value = ((RunningSampleIndex++ / HalfWavePeriod) % 2) ? ToneVolume : -ToneVolume;
              *SampleOut++ = Value;
              *SampleOut++ = Value;
            }

            GlobalSoundBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
          }
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
      }
    } else {
      // TODO: Logging
    }
  } else {
    // TDOO: Logging
  }

	return 0;
}
