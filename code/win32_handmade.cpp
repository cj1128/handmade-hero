#include <windows.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>

#include "handmade_platform.h"
#include "win32_handmade.h"

// Function Types
#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef XINPUT_GET_STATE(x_input_get_state);

#define XINPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef XINPUT_SET_STATE(x_input_set_state);

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);


XINPUT_GET_STATE(XInputGetStateStub) {
  return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;

XINPUT_SET_STATE(XInputSetStateStub) {
  return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

global_variable bool GlobalRunning = true;
global_variable bool GlobalDebugUpdateWindow = true;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSoundBuffer;
global_variable uint64 GlobalPerfCounterFrequency;

DEBUG_PLATFORM_READ_FILE(DebugPlatformReadFile) {
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

DEBUG_PLATFORM_WRITE_FILE(DebugPlatformWriteFile) {
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

DEBUG_PLATFORM_FREE_FILE_MEMORY(DebugPlatformFreeFileMemory) {
  VirtualFree(Memory, 0, MEM_RELEASE);
}

internal void
Win32BuildPathInEXEDir(
  win32_state *State,
  char *Src,
  int SrcSize,
  char *Dst,
  int DstSize
) {
  strncpy_s(Dst, DstSize, State->EXEPath, State->EXEDirLength);
  strncpy_s(Dst + State->EXEDirLength, DstSize - State->EXEDirLength, Src, SrcSize);
}

internal void
Win32GetEXEPath(win32_state *State) {
  DWORD FileNameSize = GetModuleFileNameA(NULL, State->EXEPath, sizeof(State->EXEPath));
  char *OnePastLastSlash = State->EXEPath;
  for(char *Scan = State->EXEPath + FileNameSize; ; --Scan) {
    if(*Scan == '\\') {
      OnePastLastSlash = Scan + 1;
      break;
    }
  }
  State->EXEDirLength = (int)(OnePastLastSlash - State->EXEPath);
}

internal win32_replay_buffer *
Win32GetReplayBuffer(win32_state *State, int Index) {
  return &State->ReplayBuffers[Index - 1];
}

internal void
Win32GetReplayInputFilePath(
  win32_state *State,
  int Index,
  char FilePath[],
  int FilePathSize
) {
  char FileName[30];
  sprintf_s(FileName, sizeof(FileName), "handmade_input_%d.hmi", Index);
  Win32BuildPathInEXEDir(
    State,
    FileName,
    int(strlen(FileName)),
    FilePath,
    FilePathSize
  );
}

internal void
Win32BeginInputRecording(win32_state *State, int Index) {
  State->InputRecordingIndex = Index;
  win32_replay_buffer *Buffer = Win32GetReplayBuffer(State, Index);

  char FilePath[WIN32_MAX_PATH];
  Win32GetReplayInputFilePath(State, Index, FilePath, sizeof(FilePath));
  Buffer->InputHandle = CreateFileA(
    FilePath,
    GENERIC_WRITE,
    0,
    0,
    CREATE_ALWAYS,
    FILE_ATTRIBUTE_NORMAL,
    0
  );

  CopyMemory(
    Buffer->Memory,
    State->GameMemory,
    State->MemorySize
  );
}

internal void
Win32EndInputRecording(win32_state *State) {
  win32_replay_buffer *Buffer = Win32GetReplayBuffer(State, State->InputRecordingIndex);
  CloseHandle(Buffer->InputHandle);
  State->InputRecordingIndex = 0;
}

internal void
Win32RecordInput(win32_state *State, game_input *Input) {
  win32_replay_buffer *Buffer = Win32GetReplayBuffer(State, State->InputRecordingIndex);
  DWORD BytesWritten;
  WriteFile(Buffer->InputHandle, Input, sizeof(*Input), &BytesWritten, 0);
}

internal void
Win32BeginInputPlayingBack(win32_state *State, int Index) {
  State->InputPlayingBackIndex = Index;
  win32_replay_buffer *Buffer = Win32GetReplayBuffer(State, Index);

  char FilePath[WIN32_MAX_PATH];
  Win32GetReplayInputFilePath(State, Index, FilePath, sizeof(FilePath));

  Buffer->InputHandle = CreateFileA(
    FilePath,
    GENERIC_READ,
    0,
    0,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    0
  );

  CopyMemory(State->GameMemory, Buffer->Memory, State->MemorySize);
}

internal void
Win32EndInputPlayingBack(win32_state *State) {
  win32_replay_buffer *Buffer = Win32GetReplayBuffer(State, State->InputPlayingBackIndex);
  CloseHandle(Buffer->InputHandle);
  State->InputPlayingBackIndex = 0;
}

internal void
Win32PlayBackInput(win32_state *State, game_input *Input) {
  win32_replay_buffer *Buffer = Win32GetReplayBuffer(State, State->InputPlayingBackIndex);
  DWORD BytesRead;
  if(ReadFile(Buffer->InputHandle, Input, sizeof(*Input), &BytesRead, 0)) {
    // hit the end of the stream
    if(BytesRead == 0) {
      int Index = State->InputPlayingBackIndex;
      Win32EndInputPlayingBack(State);
      Win32BeginInputPlayingBack(State, Index);
      ReadFile(Buffer->InputHandle, Input, sizeof(*Input), &BytesRead, 0);
    }

    Assert(BytesRead == sizeof(*Input));
  }
}

inline FILETIME
Win32GetFileLastWriteTime(char *FileName) {
  FILETIME Result = {};
  WIN32_FILE_ATTRIBUTE_DATA Data;

  if(GetFileAttributesExA(FileName, GetFileExInfoStandard, &Data)) {
    Result = Data.ftLastWriteTime;
  }

  return Result;
}

win32_game_code
Win32LoadGameCode(char *DLLPath, char *DLLTempPath, char *LockPah) {
  win32_game_code Result = {};

  WIN32_FILE_ATTRIBUTE_DATA Ignored;
  if(!GetFileAttributesExA(LockPah, GetFileExInfoStandard, &Ignored)) {
    Result.GameDLLLastWriteTime = Win32GetFileLastWriteTime(DLLPath);
    // CopyFile may fail the first few times
    while(1) {
      if(CopyFile(DLLPath, DLLTempPath, FALSE)) break;
      if(GetLastError() == ERROR_FILE_NOT_FOUND) break;
    }

    HMODULE Library = LoadLibraryA(DLLTempPath);

    if(Library) {
      Result.GameDLL = Library;
      Result.GameUpdateVideo = (game_update_video *)(GetProcAddress(Library, "GameUpdateVideo"));
      Result.GameUpdateAudio = (game_update_audio *)(GetProcAddress(Library, "GameUpdateAudio"));
      Result.IsValid = Result.GameUpdateVideo && Result.GameUpdateVideo;
    }
  }

  return Result;
}

void
Win32UnloadGameCode(win32_game_code *Code) {
  if(Code->GameDLL) {
    FreeLibrary(Code->GameDLL);
    Code->GameDLL = 0;
  }

  Code->IsValid = false;
  Code->GameUpdateVideo = NULL;
  Code->GameUpdateAudio = NULL;
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
Win32DebugDrawVerticalLine(
  win32_offscreen_buffer *Buffer,
  int X,
  int Top,
  int Bottom,
  int Width,
  uint32 Color
) {
  if(Top <= 0) {
    Top = 0;
  }
  if(Bottom <= 0) {
    Bottom = 0;
  }
  if(Bottom > Buffer->Height) {
    Bottom = Buffer->Height;
  }

  if(X >= 0 && X < Buffer->Width) {
    for(int Row = Top; Row < Bottom; Row++) {
      uint32 *Pixel = (uint32 *)((uint8 *)Buffer->Memory + Row * Buffer->Width * Buffer->BytesPerPixel + X * Buffer->BytesPerPixel);

      for(int Col = 0; Col < Width; Col++) {
        *Pixel++ = Color;
      }
    }
  }
}

void
Win32DebugDrawSoundMarker(
  win32_offscreen_buffer *Buffer,
  DWORD Value,
  real32 C,
  int PadX,
  int Top,
  int Bottom,
  int Width,
  uint32 Color
) {
  int X = PadX + (int)(Value * C);
  Win32DebugDrawVerticalLine(Buffer, X, Top, Bottom, 1, Color);
}

internal void
Win32DebugDrawSoundMarkers(
  win32_offscreen_buffer *Buffer,
  win32_debug_sound_marker *Markers,
  int MarkerCount,
  int CurrentMarkerIndex,
  win32_sound_output *SoundOutput
) {
  int PadX = 16;
  int PadY = 16;
  real32 C = (real32)(Buffer->Width - 2 * PadX) / (real32)SoundOutput->BufferSize;

  int LineHeight = 64;

  for(int i = 0; i < MarkerCount; i++) {
    win32_debug_sound_marker *Marker = &Markers[i];
    Assert(Marker->OutputPlayCursor < SoundOutput->BufferSize);
    Assert(Marker->OutputWriteCursor < SoundOutput->BufferSize);
    Assert(Marker->LockOffset < SoundOutput->BufferSize);
    Assert(Marker->BytesToLock < SoundOutput->BufferSize);
    Assert(Marker->FlipPlayCursor < SoundOutput->BufferSize);
    Assert(Marker->FlipWriteCursor < SoundOutput->BufferSize);

    DWORD PlayColor = 0xFFFFFFFF;
    DWORD WriteColor = 0xFFFF0000;
    DWORD ExpectedFlipColor = 0xFFFFFF00;
    DWORD PlayWindowColor = 0xFFFF00FF;

    int Top = PadY;
    int Bottom = Top + LineHeight;

    if(i == CurrentMarkerIndex) {
      Top += LineHeight + PadY;
      Bottom += LineHeight + PadY;

      Win32DebugDrawSoundMarker(Buffer, Marker->OutputPlayCursor, C, PadX, Top, Bottom, 1, PlayColor);
      Win32DebugDrawSoundMarker(Buffer, Marker->OutputWriteCursor, C, PadX, Top, Bottom, 1, WriteColor);

      Top += LineHeight + PadY;
      Bottom += LineHeight + PadY;

      Win32DebugDrawSoundMarker(Buffer, Marker->LockOffset, C, PadX, Top, Bottom, 1, PlayColor);
      Win32DebugDrawSoundMarker(Buffer, Marker->BytesToLock + Marker->LockOffset, C, PadX, Top, Bottom, 1, WriteColor);

      Top += LineHeight + PadY;
      Bottom += LineHeight + PadY;

      Win32DebugDrawSoundMarker(Buffer, Marker->ExpectedFlipPlayCursor, C, PadX, Top, Bottom, 1, ExpectedFlipColor);
    }

    Win32DebugDrawSoundMarker(Buffer, Marker->FlipPlayCursor, C, PadX, Top, Bottom, 1, PlayColor);
    Win32DebugDrawSoundMarker(Buffer, Marker->FlipPlayCursor + 480 * SoundOutput->BytesPerSample, C, PadX, Top, Bottom, 1, PlayWindowColor);
    Win32DebugDrawSoundMarker(Buffer, Marker->FlipWriteCursor, C, PadX, Top, Bottom, 1, WriteColor);
  }
}

internal void
Win32ProcessKeyboardButtonState(
  game_button_state *NewButtonState,
  bool32 IsDown
) {
  if(NewButtonState->IsEndedDown != IsDown) {
    NewButtonState->IsEndedDown = IsDown;
    NewButtonState->HalfTransitionCount++;
  }
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
Win32ProcessMessages(win32_state *Win32State, game_controller_input *KeyboardController) {
  MSG Message = {};

  while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
    switch(Message.message) {
      case WM_SYSKEYUP:
      case WM_SYSKEYDOWN:
      case WM_KEYUP:
      case WM_KEYDOWN: {
        uint32 VKCode = (uint32)Message.wParam;
        bool WasDown = (Message.lParam & (1 << 30)) != 0;
        bool IsDown = (Message.lParam & (1 << 31)) == 0;

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

            case 'P': {
              if(Message.message == WM_KEYDOWN) {
                GlobalDebugUpdateWindow = !GlobalDebugUpdateWindow;
              }
            };

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

            case 'L': {
              if(IsDown) {
                if(Win32State->InputRecordingIndex == 0) {
                  if(Win32State->InputPlayingBackIndex == 0) {
                    Win32BeginInputRecording(Win32State, 1);
                  } else {
                    Win32EndInputPlayingBack(Win32State);
                    // Reset all buttons
                    for(int i = 0; i < ArrayCount(KeyboardController->Buttons); i++) {
                      KeyboardController->Buttons[i] = {};
                    }
                  }
                } else {
                  Win32EndInputRecording(Win32State);
                  Win32BeginInputPlayingBack(Win32State, 1);
                }
              }
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
        // OutputDebugStringA("Set cooperative level ok\n");
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
          // OutputDebugStringA("Create primary buffer ok\n");
          if(SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat))) {
            // OutputDebugStringA("Primary buffer set format ok\n");
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
          // OutputDebugStringA("Secondary buffer created\n");
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
  Buffer->Info.bmiHeader.biHeight = Buffer->Height;
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
  int Top = 10;
  int Left = 10;

  // four gutters
  PatBlt(DeviceContext, 0, 0, WindowWidth, Top, BLACKNESS);
  PatBlt(DeviceContext, 0, Top, Left, Buffer->Height, BLACKNESS);
  PatBlt(DeviceContext, 0, Top + Buffer->Height, WindowWidth, WindowHeight - Buffer->Height - Top, BLACKNESS);
  PatBlt(DeviceContext, Left + Buffer->Width, Top, WindowWidth - Left - Buffer->Width, Buffer->Height, BLACKNESS);

  StretchDIBits(
    DeviceContext,
    Top, Left, Buffer->Width, Buffer->Height, // dest
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

inline real32
Win32GetSecondsElapsed(uint64 Start, uint64 End) {
  real32 Result = 0;
  Result = (real32)(End - Start) / (real32)GlobalPerfCounterFrequency;
  return Result;
}

inline uint64
Win32GetPerfCounter() {
  LARGE_INTEGER Result;
  QueryPerformanceCounter(&Result);
  return Result.QuadPart;
}

#define SoundFrameLatency 3
int CALLBACK
WinMain(
  HINSTANCE Instance,
  HINSTANCE PrevInstance,
  LPSTR     CmdLine,
  int       ShowCmd
) {
  bool32 SleepIsGranular = timeBeginPeriod(1) == TIMERR_NOERROR ;

  LARGE_INTEGER CounterPerSecond;
  QueryPerformanceFrequency(&CounterPerSecond);
  GlobalPerfCounterFrequency = CounterPerSecond.QuadPart;

  Win32LoadXInput();

  WNDCLASS WindowClass = {};

  WindowClass.style = CS_HREDRAW | CS_VREDRAW;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = Instance;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";

  if(RegisterClass(&WindowClass)) {
    thread_context Thread = {};
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
      HDC DeviceContext = GetDC(Window);
      int MonitorRefreshHz = GetDeviceCaps(DeviceContext, VREFRESH);
      int GameUpdateHz = MonitorRefreshHz / 2;
      real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;

      win32_sound_output SoundOutput = {};
      SoundOutput.SamplesPerSecond = 48000;
      SoundOutput.BytesPerSample = 4;
      SoundOutput.SafetyBytes = (SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample) / GameUpdateHz / 3;
      SoundOutput.BufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
      SoundOutput.ToneVolume = 3000;
      SoundOutput.RunningSampleIndex = 0;
      int16 *SoundMemory = (int16 *)VirtualAlloc(0, SoundOutput.BufferSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);

      game_memory Memory = {};
      Memory.DebugPlatformReadFile = DebugPlatformReadFile;
      Memory.DebugPlatformWriteFile = DebugPlatformWriteFile;
      Memory.DebugPlatformFreeFileMemory = DebugPlatformFreeFileMemory;
      Memory.PermanentStorageSize = Megabytes(64);
      Memory.TransientStorageSize = Megabytes(1);
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

      win32_state Win32State = {};
      Win32State.MemorySize = Memory.PermanentStorageSize + Memory.TransientStorageSize;

      for(int i = 0; i < ArrayCount(Win32State.ReplayBuffers); i++) {
        Win32State.ReplayBuffers[i].Memory = VirtualAlloc(
          0,
          Win32State.MemorySize,
          MEM_COMMIT|MEM_RESERVE,
          PAGE_READWRITE
        );
        Assert(Win32State.ReplayBuffers[i].Memory != 0);
      }

      if(Memory.PermanentStorage && SoundMemory) {
        Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.BufferSize);
        Win32ClearSoundBuffer(&SoundOutput);
        GlobalSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);

// write cursor - last write cursor: 1920 bytes = 480 samples
#if 0
        while(GlobalRunning) {
          DWORD PlayCursor, WriteCursor;
          GlobalSoundBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);
          char Buffer[256];
          sprintf_s(Buffer, sizeof(Buffer), "PC: %d, WC: %d\n", PlayCursor, WriteCursor);
          OutputDebugStringA(Buffer);
        }
#endif
        Win32State.GameMemory = Memory.PermanentStorage;
        Win32State.MemorySize = Memory.PermanentStorageSize;

        Win32ResizeDIBSection(&GlobalBackBuffer, 960, 540);

        game_input Input[2] = {};
        game_input *OldInput = &Input[0];
        game_input *NewInput = &Input[1];

        uint64 LastCounter = Win32GetPerfCounter();
        uint64 FlipCounter = Win32GetPerfCounter();
        uint64 LastCycleCounter = __rdtsc();

        bool32 SoundIsValid = false;
#if 0
        int DebugSoundMarkerIndex = 0;
#endif
        win32_debug_sound_marker DebugSoundMarkers[15] = {};

        Win32GetEXEPath(&Win32State);

        char GameDLLName[] = "handmade.dll";
        char GameDLLPath[WIN32_MAX_PATH];
        Win32BuildPathInEXEDir(
          &Win32State,
          GameDLLName,
          sizeof(GameDLLName) - 1,
          GameDLLPath,
          sizeof(GameDLLPath)
        );

        char GameTempDLLName[] = "handmade_temp.dll";
        char GameTempDLLPath[WIN32_MAX_PATH];
        Win32BuildPathInEXEDir(
          &Win32State,
          GameTempDLLName,
          sizeof(GameTempDLLName) - 1,
          GameTempDLLPath,
          sizeof(GameTempDLLPath)
        );

        char GameLockFileName[] = "lock.tmp";
        char GameLockFilePath[WIN32_MAX_PATH];
        Win32BuildPathInEXEDir(
          &Win32State,
          GameLockFileName,
          sizeof(GameLockFileName) - 1,
          GameLockFilePath,
          sizeof(GameLockFilePath)
        );

        win32_game_code Game = Win32LoadGameCode(GameDLLPath, GameTempDLLPath, GameLockFilePath);

        while(GlobalRunning) {
          FILETIME NewDLLWriteTime = Win32GetFileLastWriteTime(GameDLLPath);
          if(CompareFileTime(&NewDLLWriteTime, &Game.GameDLLLastWriteTime) != 0) {
            Win32UnloadGameCode(&Game);
            Game = Win32LoadGameCode(GameDLLPath, GameTempDLLPath, GameLockFilePath);
          }

          NewInput->dt = TargetSecondsPerFrame;

          game_controller_input *NewKeyboardController = &NewInput->Controllers[0];
          game_controller_input *OldKeyboardController = &OldInput->Controllers[0];
          *NewKeyboardController = {};
          NewKeyboardController->IsConnected = true;

          for(int i = 0; i < ArrayCount(NewKeyboardController->Buttons); i++) {
            NewKeyboardController->Buttons[i].IsEndedDown = OldKeyboardController->Buttons[i].IsEndedDown;
          }

          Win32ProcessMessages(&Win32State, NewKeyboardController);

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
              NewController->IsAnalog = OldController->IsAnalog;
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

          // Update video
          {
            game_offscreen_buffer Buffer = {};
            Buffer.Memory = GlobalBackBuffer.Memory;
            Buffer.BytesPerPixel = GlobalBackBuffer.BytesPerPixel;
            Buffer.Width = GlobalBackBuffer.Width;
            Buffer.Height = GlobalBackBuffer.Height;
            Buffer.Pitch = Buffer.Width * Buffer.BytesPerPixel;

            if(Win32State.InputRecordingIndex != 0) {
              Win32RecordInput(&Win32State, NewInput);
            }

            if(Win32State.InputPlayingBackIndex != 0) {
              Win32PlayBackInput(&Win32State, NewInput);
            }

            // Collect mouse info
            {
              POINT Point;
              GetCursorPos(&Point);
              ScreenToClient(Window, &Point);
              NewInput->MouseX = int32(Point.x);
              NewInput->MouseY = int32(Point.y);


              NewInput->MouseButtons[0].IsEndedDown = GetKeyState(VK_LBUTTON) & (1 << 15);
              NewInput->MouseButtons[1].IsEndedDown = GetKeyState(VK_RBUTTON) & (1 << 15);
              NewInput->MouseButtons[2].IsEndedDown = GetKeyState(VK_MBUTTON) & (1 << 15);
              NewInput->MouseButtons[3].IsEndedDown = GetKeyState(VK_XBUTTON1) & (1 << 15);
              NewInput->MouseButtons[4].IsEndedDown = GetKeyState(VK_XBUTTON2) & (1 << 15);
            }

            if(Game.GameUpdateVideo) {
              Game.GameUpdateVideo(&Thread, &Memory, NewInput, &Buffer);
            }
          }

          // Update Audio
          {
            uint64 AudioCounter = Win32GetPerfCounter();
            real32 FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipCounter, AudioCounter);

            DWORD PlayCursor, WriteCursor;
            if(GlobalSoundBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK) {
              if(!SoundIsValid) {
                SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
                SoundIsValid = true;
              }

              DWORD LockOffset = SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample % SoundOutput.BufferSize;
              DWORD SoundBytesPerFrame = (SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample) / GameUpdateHz;
              real32 SecondsLeftUntilFlip = TargetSecondsPerFrame - FromBeginToAudioSeconds;
              DWORD ExpectedBytesUntilFlip = (DWORD)(SecondsLeftUntilFlip / TargetSecondsPerFrame * (real32)SoundBytesPerFrame);
              DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedBytesUntilFlip;
              DWORD SafeWriteCursor = WriteCursor;
              if(SafeWriteCursor < PlayCursor) {
                SafeWriteCursor += SoundOutput.BufferSize;
              }
              Assert(SafeWriteCursor >= PlayCursor);
              SafeWriteCursor += SoundOutput.SafetyBytes;
              bool32 AudioIsLowLatency = SafeWriteCursor <= ExpectedFrameBoundaryByte;
              DWORD TargetCursor = 0;
              if(AudioIsLowLatency) {
                TargetCursor = ExpectedFrameBoundaryByte + SoundBytesPerFrame;
              } else {
                TargetCursor = WriteCursor + SoundBytesPerFrame + SoundOutput.SafetyBytes;
              }
              TargetCursor = TargetCursor % SoundOutput.BufferSize;

              DWORD BytesToLock = 0;
              if(LockOffset > TargetCursor) {
                BytesToLock = SoundOutput.BufferSize - LockOffset + TargetCursor;
              } else {
                BytesToLock = TargetCursor - LockOffset;
              }

              game_sound_buffer SoundBuffer = {};
              SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
              SoundBuffer.SampleCount = BytesToLock / SoundOutput.BytesPerSample;
              SoundBuffer.ToneVolume = SoundOutput.ToneVolume;
              SoundBuffer.Memory = SoundMemory;
              if(Game.GameUpdateAudio) {
                Game.GameUpdateAudio(&Thread, &Memory, &SoundBuffer);
              }

#if 0
              win32_debug_sound_marker *Marker = &DebugSoundMarkers[DebugSoundMarkerIndex];
              Marker->OutputPlayCursor = PlayCursor;
              Marker->OutputWriteCursor = WriteCursor;
              Marker->LockOffset = LockOffset;
              Marker->BytesToLock = BytesToLock;
              Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte;

              DWORD UnwrappedWriteCursor = WriteCursor;
              if(UnwrappedWriteCursor < PlayCursor) {
                UnwrappedWriteCursor += SoundOutput.BufferSize;
              }
              DWORD AudioLantencyBytes = UnwrappedWriteCursor - PlayCursor;
              real32 AudioLatencySeconds = (real32)(AudioLantencyBytes / SoundOutput.BytesPerSample) / (real32)SoundOutput.SamplesPerSecond;

              char AudioTextBuffer[256];
              sprintf_s(AudioTextBuffer, "LockOffset: %d, TargetCursor: %d, BytesToLock: %d, PlayCursor: %d, WriteCursor: %d, AudioLatencyBytes: %d, AudioLatencySeconds: %f\n", LockOffset, TargetCursor, BytesToLock, PlayCursor, WriteCursor, AudioLantencyBytes, AudioLatencySeconds);
              // OutputDebugStringA(AudioTextBuffer);
#endif

              Win32FillSoundBuffer(&SoundOutput, &SoundBuffer, LockOffset, BytesToLock);
            } else {
              SoundIsValid = false;
            }
          }

          // Lock the frame rate
          real32 SecondsElapsed = Win32GetSecondsElapsed(LastCounter, Win32GetPerfCounter());
          if(SecondsElapsed < TargetSecondsPerFrame) {
            if(SleepIsGranular) {
              DWORD SleepMS = (DWORD)(1000 * (TargetSecondsPerFrame - SecondsElapsed));
              if(SleepMS > 0) {
                Sleep(SleepMS);
              }
            }

            while(SecondsElapsed < TargetSecondsPerFrame) {
              SecondsElapsed = Win32GetSecondsElapsed(LastCounter, Win32GetPerfCounter());
            }
          } else {
            // We missed a frame
            // TODO: logging
          }

          uint64 EndCounter = Win32GetPerfCounter();

#if 0
          Win32DebugDrawSoundMarkers(
            &GlobalBackBuffer,
            DebugSoundMarkers,
            ArrayCount(DebugSoundMarkers),
            DebugSoundMarkerIndex - 1, // maybe -1, we don't care
            &SoundOutput
          );
#endif

          if(GlobalDebugUpdateWindow) {
            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            HDC DC = GetDC(Window);
            Win32UpdateWindow(
              DC,
              Dimension.Width,
              Dimension.Height,
              &GlobalBackBuffer
            );
          }
          FlipCounter = Win32GetPerfCounter();

#if 0
          {
            DWORD PlayCursor, WriteCursor;
            if(SUCCEEDED(GlobalSoundBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor))) {
            win32_debug_sound_marker *Marker = &DebugSoundMarkers[DebugSoundMarkerIndex];
            Marker->FlipPlayCursor = PlayCursor;
            Marker->FlipWriteCursor = WriteCursor;
            }
          }
          DebugSoundMarkerIndex++;
          if(DebugSoundMarkerIndex == ArrayCount(DebugSoundMarkers)) {
            DebugSoundMarkerIndex = 0;
          }
#endif

          game_input *Tmp = OldInput;
          OldInput = NewInput;
          NewInput = Tmp;

          // Performance counter
          uint64 CurrentCycleCounter = __rdtsc();
#if 1

          int64 CounterElapsed = EndCounter - LastCounter;
          uint64 CycleElapsed = CurrentCycleCounter - LastCycleCounter;

          real32 MSPerFrame = 1000.0f * (real32)CounterElapsed / (real32)CounterPerSecond.QuadPart;
          real32 FPS = (real32)CounterPerSecond.QuadPart / (real32)CounterElapsed;
          real32 MCPF = (real32)CycleElapsed / (1000.0f * 1000.0f);

          char OutputBuffer[256];
          sprintf_s(OutputBuffer, sizeof(OutputBuffer), "ms/f: %.2f,  fps: %.2f,  mc/f: %.2f\n", MSPerFrame, FPS, MCPF);
          OutputDebugStringA(OutputBuffer);
#endif

          LastCounter = EndCounter;
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
