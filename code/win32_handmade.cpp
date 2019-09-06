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
global_variable LPDIRECTSOUNDBUFFER GlobalSoundBuffer;

debug_read_file_result DebugPlatformReadFile(char *FileName) {
  debug_read_file_result Result = {};

  HANDLE File = CreateFileA(
    FileName,
    GENERIC_READ,
    0,
    0,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    0
  );

  LARGE_INTEGER FileSize;

  if(File != INVALID_HANDLE_VALUE) {
    if(GetFileSizeEx(File, &FileSize)) {
      uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
      void *Memory = VirtualAlloc(0, FileSize32, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
      if(Memory) {
        DWORD BytesRead;
        if(ReadFile(File, Memory, FileSize32, &BytesRead, 0) && BytesRead == FileSize32){
          Result.Size = FileSize32;
          Result.Memory = Memory;
        } else {
          // TODO: logging
        }
      } else {
        // TODO: logging
      }
    } else {
      // TODO: logging
    }

    CloseHandle(File);
  } else {
    // TODO: logging
  }

  return Result;
}

bool32
DebugPlatformWriteFile(char *FileName, void *Memory, uint32 FileSize) {
  bool32 Result = false;

  HANDLE File = CreateFileA(
    FileName,
    GENERIC_WRITE,
    0,
    0,
    CREATE_ALWAYS,
    FILE_ATTRIBUTE_NORMAL,
    0
  );

  if(File != INVALID_HANDLE_VALUE) {
    DWORD BytesWritten;
    if(WriteFile(File, Memory, FileSize, &BytesWritten, 0)) {
      Result = BytesWritten == FileSize;
    } else {
      // TODO: logging
    }

    CloseHandle(File);
  } else {
    // TODO: logging
  }

  return Result;
}

void
DebugPlatformFreeFileMemory(void *Memory) {
  VirtualFree(Memory, 0, MEM_RELEASE);
}

internal real32
Win32ProcessStickValue(int16 Value, int16 DeadValue) {
  real32 Result = 0;

  if(Value < -DeadValue) {
    Result = (real32)(Value + DeadValue) / (real32)(32768 - DeadValue);
  }

  if(Value > DeadValue) {
    Result = (real32)(Value - DeadValue) / (real32)(32767 - DeadValue);
  }

  return Result;
}

internal void
Win32ProcessKeyboardButtonState(
  game_button_state *NewButtonState,
  bool32 IsDown
) {
  Assert(NewButtonState->IsEndedDown != IsDown)
  NewButtonState->IsEndedDown = IsDown;
  NewButtonState->HalfTransitionCount++;
}

internal void
Win32ProcessXInputButtonState(
  game_button_state *NewButtonState,
  game_button_state *OldButtonState,
  DWORD Buttons,
  DWORD ButtonBit
) {
  NewButtonState->HalfTransitionCount = OldButtonState->IsEndedDown == NewButtonState->IsEndedDown ? 0 : 1;
  NewButtonState->IsEndedDown = Buttons & ButtonBit;
}

internal void
Win32ProcessMessages(game_controller_input *KeyboardController) {
  MSG Message = {};

  while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
    switch(Message.message) {
      case WM_SYSKEYUP:
      case WM_SYSKEYDOWN:
      case WM_KEYUP:
      case WM_KEYDOWN: {
        uint32 VKCode = (uint32)Message.wParam;
        bool32 WasDown = (Message.lParam & (1 << 30)) != 0;
        bool32 IsDown = (Message.lParam & (1 << 31)) == 0;

        if(WasDown != IsDown) {
          switch(VKCode) {
            case VK_F4: {
              bool32 IsAltDown = Message.lParam & (1 << 29);
              if(IsAltDown) {
                GlobalRunning = false;
              }
            } break;

            case 'W': {
              Win32ProcessKeyboardButtonState(&KeyboardController->MoveUp, IsDown);
            } break;
            case 'A': {
              Win32ProcessKeyboardButtonState(&KeyboardController->MoveLeft, IsDown);
            } break;
            case 'S': {
              Win32ProcessKeyboardButtonState(&KeyboardController->MoveDown, IsDown);
            } break;
            case 'D': {
              Win32ProcessKeyboardButtonState(&KeyboardController->MoveRight, IsDown);
            } break;

            case VK_UP: {
              Win32ProcessKeyboardButtonState(&KeyboardController->ActionUp, IsDown);
            } break;

            case VK_DOWN: {
              Win32ProcessKeyboardButtonState(&KeyboardController->ActionDown, IsDown);
            } break;

            case VK_LEFT: {
              Win32ProcessKeyboardButtonState(&KeyboardController->ActionLeft, IsDown);
            } break;

            case VK_RIGHT: {
              Win32ProcessKeyboardButtonState(&KeyboardController->ActionRight, IsDown);
            } break;

            case VK_ESCAPE: {
              Win32ProcessKeyboardButtonState(&KeyboardController->Back, IsDown);
            } break;

            case VK_SPACE: {
              Win32ProcessKeyboardButtonState(&KeyboardController->Start, IsDown);
            } break;

            // left shoulder
            case 'Q': {
              Win32ProcessKeyboardButtonState(&KeyboardController->LeftShoulder, IsDown);
            } break;

            // right shoulder
            case 'E': {
              Win32ProcessKeyboardButtonState(&KeyboardController->RightShoulder, IsDown);
            } break;
          }
        }
      } break;

      default: {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
      } break;
    }
  }
}

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

    for(DWORD ByteIndex = 0; ByteIndex < Region1Size; ByteIndex++) {
      *Out++ = 0;
    }

    Out = (uint8 *)Region2;

    for(DWORD ByteIndex = 0; ByteIndex < Region2Size; ByteIndex++) {
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

    for(DWORD SampleIndex = 0; SampleIndex < Region1Size / SoundOutput->BytesPerSample; SampleIndex++) {
      *Dest++ = *Source++;
      *Dest++ = *Source++;
      SoundOutput->RunningSampleIndex++;
    }

    Dest = (int16 *)Region2;
    for(DWORD SampleIndex = 0; SampleIndex < Region2Size / SoundOutput->BytesPerSample; SampleIndex++) {
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
Win32ResizeDIBSection(
  win32_offscreen_buffer *Buffer,
  int Width,
  int Height
) {
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
  Buffer->Info.bmiHeader.biBitCount = (WORD)(Buffer->BytesPerPixel * 8);
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
      Assert(!"no keyboard message here")
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

      game_memory Memory = {};
      Memory.PermanentStorageSize = Megabytes(64);
      Memory.TransientStorageSize = Gigabytes((uint64)1);
#ifdef HANDMADE_INTERNAL
      LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
      LPVOID BaseAddress = 0;
#endif
      Memory.PermanentStorage = VirtualAlloc(
        BaseAddress,
        (size_t)(Memory.PermanentStorageSize + Memory.TransientStorageSize),
        MEM_COMMIT|MEM_RESERVE,
        PAGE_READWRITE
      );
      Memory.TransientStorage = (uint8 *)Memory.PermanentStorage + Memory.PermanentStorageSize;

      if(Memory.PermanentStorage && SoundMemory) {
        Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.BufferSize);
        Win32ClearSoundBuffer(&SoundOutput);
        GlobalSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);

        Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

        game_input Input[2] = {};
        game_input *OldInput = &Input[0];
        game_input *NewInput = &Input[1];

        LARGE_INTEGER LastCounter;
        uint64 LastCycleCounter = __rdtsc();
        QueryPerformanceCounter(&LastCounter);
        while(GlobalRunning) {
          game_controller_input *NewKeyboardController = &NewInput->Controllers[0];
          game_controller_input *OldKeyboardController = &OldInput->Controllers[0];
          *NewKeyboardController = {};
          NewKeyboardController->IsConnected = true;

          for(int i = 0; i < ArrayCount(NewKeyboardController->Buttons); i++) {
            NewKeyboardController->Buttons[i].IsEndedDown = OldKeyboardController->Buttons[i].IsEndedDown;
          }

          Win32ProcessMessages(NewKeyboardController);

          int ControllerCount = XUSER_MAX_COUNT;
          if(ControllerCount > (ArrayCount(NewInput->Controllers) - 1)) {
            ControllerCount = ArrayCount(NewInput->Controllers) - 1;
          }

          for(int i=0; i< ControllerCount; i++) {
            XINPUT_STATE state;

            game_controller_input *OldController = &OldInput->Controllers[i + 1];
            game_controller_input *NewController = &NewInput->Controllers[i + 1];

            ZeroMemory(&state, sizeof(XINPUT_STATE));

            if(XInputGetState(i, &state) == ERROR_SUCCESS ) {
              NewController->IsConnected = true;
              XINPUT_GAMEPAD *Pad = &state.Gamepad;

              NewController->StickAverageX = Win32ProcessStickValue(
                Pad->sThumbLX,
                XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE
              );
              NewController->StickAverageY = Win32ProcessStickValue(
                Pad->sThumbLY,
                XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE
              );

              if(NewController->StickAverageX != 0.0f || NewController->StickAverageY != 0.0f) {
                NewController->IsAnalog = true;
              }

              if(Pad->wButtons &XINPUT_GAMEPAD_DPAD_UP) {
                NewController->StickAverageY = 1.0f;
                NewController->IsAnalog = false;
              }
              if(Pad->wButtons &XINPUT_GAMEPAD_DPAD_DOWN) {
                NewController->StickAverageY = -1.0f;
                NewController->IsAnalog = false;
              }
              if(Pad->wButtons &XINPUT_GAMEPAD_DPAD_RIGHT) {
                NewController->StickAverageY = 1.0f;
                NewController->IsAnalog = false;
              }
              if(Pad->wButtons &XINPUT_GAMEPAD_DPAD_LEFT) {
                NewController->StickAverageY = -1.0f;
                NewController->IsAnalog = false;
              }

              real32 Threshold = 0.5f;
              Win32ProcessXInputButtonState(
                &NewController->MoveUp, &OldController->MoveUp,
                (NewController->StickAverageY > Threshold ? 1 : 0), 1
              );
              Win32ProcessXInputButtonState(
                &NewController->MoveDown, &OldController->MoveDown,
                (NewController->StickAverageY < -Threshold ? 1 : 0), 1
              );
              Win32ProcessXInputButtonState(
                &NewController->MoveRight, &OldController->MoveRight,
                (NewController->StickAverageX > Threshold ? 1 : 0), 1
              );
              Win32ProcessXInputButtonState(
                &NewController->MoveLeft, &OldController->MoveLeft,
                (NewController->StickAverageX < -Threshold ? 1 : 0), 1
              );

              Win32ProcessXInputButtonState(
                &NewController->ActionDown, &OldController->ActionDown,
                Pad->wButtons, XINPUT_GAMEPAD_A
              );
              Win32ProcessXInputButtonState(
                &NewController->ActionRight, &OldController->ActionRight,
                Pad->wButtons, XINPUT_GAMEPAD_B
              );
              Win32ProcessXInputButtonState(
                &NewController->ActionLeft, &OldController->ActionLeft,
                Pad->wButtons, XINPUT_GAMEPAD_X
              );
              Win32ProcessXInputButtonState(
                &NewController->ActionUp, &OldController->ActionUp,
                Pad->wButtons, XINPUT_GAMEPAD_Y
              );
              Win32ProcessXInputButtonState(
                &NewController->LeftShoulder, &OldController->LeftShoulder,
                Pad->wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER
              );
              Win32ProcessXInputButtonState(
                &NewController->RightShoulder, &OldController->RightShoulder,
                Pad->wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER
              );
              Win32ProcessXInputButtonState(
                &NewController->Start, &OldController->Start,
                Pad->wButtons, XINPUT_GAMEPAD_START
              );
              Win32ProcessXInputButtonState(
                &NewController->Back, &OldController->Back,
                Pad->wButtons, XINPUT_GAMEPAD_BACK
              );
            }
            else{
              // Controller is not connected
            }
          }

          DWORD BytesToLock = 0;
          DWORD LockOffset = 0;
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
          SoundBuffer.ToneVolume = SoundOutput.ToneVolume;
          SoundBuffer.Memory = SoundMemory;

          GameUpdateAndRender(&Memory, NewInput, &Buffer, &SoundBuffer);

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

          game_input *Tmp = OldInput;
          OldInput = NewInput;
          NewInput = Tmp;

          // Performance counter
          uint64 CurrentCycleCounter = __rdtsc();
          LARGE_INTEGER CurrentCounter;
          QueryPerformanceCounter(&CurrentCounter);

          int64 CounterElapsed = CurrentCounter.QuadPart - LastCounter.QuadPart;
          uint64 CycleElapsed = CurrentCycleCounter - LastCycleCounter;

          real32 MSPerFrame = 1000.0f * (real32)CounterElapsed / (real32)CounterPerSecond.QuadPart;
          real32 FPS = (real32)CounterPerSecond.QuadPart / (real32)CounterElapsed;
          real32 MCPF = (real32)CycleElapsed / (1000.0f * 1000.0f);

          char OutputBuffer[256];
          sprintf_s(OutputBuffer, sizeof(OutputBuffer), "ms/f: %.2f,  fps: %.2f,  mc/f: %.2f\n", MSPerFrame, FPS, MCPF);
          OutputDebugStringA(OutputBuffer);

          LastCounter = CurrentCounter;
          LastCycleCounter = CurrentCycleCounter;
        }
      } else {
        // TODO: Logging
      }
    } else {
      // TODO: Logging
    }
  } else {
    // TDOO: Logging
  }

  return 0;
}
