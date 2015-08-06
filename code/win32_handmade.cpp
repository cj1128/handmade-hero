#include <windows.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>
#include <stdlib.h>

#include "handmade.h"
#include "win32_handmade.h"

/*
THIS IS NOT A FINAL PLATFORM LAYER!
- Saved game locations
- Getting a handle to our own exectuable file
- Asset loading path
- Threading (launch a thread)
- Raw Input (support for multiple keyborad)
- Sleep/timeBeginPeriod
- ClipCursor() (for multimonitor support)
- FullScreen support
- WM_SETCURSOR (control cursor visibility)
- QueryCancelAutoplay
- WM_ACTIVATEAPP (for when we are not the active application)
- Blit speed improvements (BitBlt)
- Hardware Acceleration (OpenGl or Direct3D or Both)
- GetKeyboardLayout (for French keyboards, international WASD support)

  JUST A PARTIAL LIST OF STUFF!
 */

global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64 GlobalPerfCounterFrequency;
global_variable bool GlobalPause;

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
global_variable x_input_get_state *XInputGetState_ = 0;

// NOTE(casey): XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
global_variable x_input_set_state *XInputSetState_ = 0;

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal int
GetStringLength(char *str)
{
  int length = 0;
  while(*str++)
  {
    length++;
  }
  return length;
}

internal void
CatStrings(size_t LeftLength, char *Left,
  size_t RightLength, char *Right,
  char *Dest)
{
  for(int i = 0; i< LeftLength; i++)
  {
    *Dest++ = *Left++;
  }
  for(int i = 0; i < RightLength; i++)
  {
    *Dest++ = *Right++;
  }
  *Dest = '\0';
}

internal void
GetInputFilePath(win32_state *State, char *FileName, char *FilePath)
{
  CatStrings(State->OnePastLastEXEPathSlash - State->EXEPath,
    State->EXEPath,
    GetStringLength(FileName),
    FileName,
    FilePath
  );
}

internal void
Win32BeginRecordingInput(win32_state *State, int InputRecordingIndex)
{
  State->InputRecordingIndex = InputRecordingIndex;
  char *FileName = "looped_input.hmi";
  char FilePath[MAX_PATH];
  GetInputFilePath(State, FileName, FilePath);
  State->RecordingHandle = CreateFileA(FilePath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

  DWORD BytesToWrite = (DWORD)State->TotalSize;
  DWORD BytesWritten;
  WriteFile(State->RecordingHandle, State->GameMemoryBlock, BytesToWrite, &BytesWritten, 0);
}

internal void
Win32RecordInput(win32_state *State, game_input *Input)
{
  DWORD BytesWritten;
  WriteFile(State->RecordingHandle, Input, sizeof(*Input), &BytesWritten, 0);
}

internal void
Win32EndRecordingInput(win32_state *Win32State)
{
  CloseHandle(Win32State->RecordingHandle);
  Win32State->InputRecordingIndex = 0;
}

internal void
Win32BeginInputPlayingback(win32_state *State, int InputPlayingbackIndex)
{
  State->InputPlayingbackIndex = InputPlayingbackIndex;
  char *FileName = "looped_input.hmi";
  char FilePath[MAX_PATH];
  GetInputFilePath(State, FileName, FilePath);
  State->PlayingbackHandle = CreateFileA(FilePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

  DWORD BytesToRead = (DWORD)State->TotalSize;
  DWORD BytesRead;
  ReadFile(State->PlayingbackHandle, State->GameMemoryBlock, BytesToRead, &BytesRead, 0);
}

internal void
Win32EndInputPlayingback(win32_state *Win32State)
{
  CloseHandle(Win32State->PlayingbackHandle);
  Win32State->InputPlayingbackIndex = 0;
}

internal void
Win32PlaybackInput(win32_state *Win32State, game_input *Input)
{
  DWORD BytesRead;
  if(ReadFile(Win32State->PlayingbackHandle, Input, sizeof(*Input), &BytesRead, 0))
  {
    if(BytesRead == 0)
    {
      int PlayingbackIndex = Win32State->InputPlayingbackIndex;
      Win32EndInputPlayingback(Win32State);
      Win32BeginInputPlayingback(Win32State, PlayingbackIndex);
      ReadFile(Win32State->PlayingbackHandle, Input, sizeof(*Input), &BytesRead, 0);
    }
  }
}



internal FILETIME
Win32GetLastWriteTime(char *FilePath)
{
  WIN32_FILE_ATTRIBUTE_DATA Data;
  GetFileAttributesEx(FilePath,
    GetFileExInfoStandard, &Data);
  return Data.ftLastWriteTime;
}

internal win32_game_code
Win32LoadGameCode(char *SourceDLLPath, char *TempDLLPath)
{
  win32_game_code Result = {};

  Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLPath);
  CopyFile(SourceDLLPath, TempDLLPath, FALSE);
  Result.GameCodeDLL = LoadLibraryA(TempDLLPath);
  if(Result.GameCodeDLL)
  {
    Result.GameUpdateVideo = (game_update_video *)GetProcAddress(Result.GameCodeDLL, "GameUpdateVideo");
    Result.GameUpdateAudio = (game_update_audio *)GetProcAddress(Result.GameCodeDLL, "GameUpdateAudio");

    Result.IsValid = (Result.GameUpdateVideo && Result.GameUpdateAudio);

    if(!Result.IsValid)
    {
      OutputDebugStringA("Can't load Functions in module!");
      exit(1);
    }
  }
  else
  {
    OutputDebugStringA("Error! Can't load game module!");
    exit(1);
  }
  return Result;
}

internal void
Win32UnloadGameCode(win32_game_code *GameCode)
{
  if(GameCode->GameCodeDLL)
  {
    FreeLibrary(GameCode->GameCodeDLL);
    GameCode->GameCodeDLL = 0;
  }
  GameCode->IsValid = false;
  GameCode->GameUpdateVideo = 0;
  GameCode->GameUpdateAudio = 0;
}

#if 0
internal void
Win32DebugDrawVertical(win32_offscreen_buffer *BackBuffer, int X, int Top, int Bottom, uint32 Color)
{
  if(Top <= 0)
  {
    Top = 0;
  }
  if(Bottom >= BackBuffer->Height)
  {
    Bottom = BackBuffer->Height;
  }

  if((X >= 0 ) && (X <= BackBuffer->Width))
  {
    uint8 *Pixel = ((uint8 *)BackBuffer->Memory + X*BackBuffer->BytesPerPixel + Top * BackBuffer->Pitch);
    for(int Y = Top;
      Y < Bottom;
      Y++)
    {
      *(uint32 *)Pixel = Color;
      Pixel += BackBuffer->Pitch;
    }
  }
}

internal void
Win32DrawSoundBufferMarker(win32_offscreen_buffer *BackBuffer, win32_sound_output *SoundOutput, real32 C, int PadX, int Top, int Bottom, DWORD Value, uint32 Color)
{
  real32 XReal32 = C * Value;
  int X = (int)(XReal32 + PadX);
  Win32DebugDrawVertical(BackBuffer, X, Top, Bottom, Color);
}

internal void
Win32DebugSyncDisplay(win32_offscreen_buffer *Backbuffer,
       int MarkerCount, win32_debug_time_marker *Markers,
       int CurrentMarkerIndex,
       win32_sound_output *SoundOutput, real32 TargetSecondsPerFrame)
{
  int PadX = 16;
  int PadY = 16;

  int LineHeight = 64;

  real32 C = (real32)(Backbuffer->Width - 2*PadX) / (real32)SoundOutput->SecondaryBufferSize;
  for(int MarkerIndex = 0;
    MarkerIndex < MarkerCount;
    ++MarkerIndex)
  {
    win32_debug_time_marker *ThisMarker = &Markers[MarkerIndex];
    Assert(ThisMarker->OutputPlayCursor < SoundOutput->SecondaryBufferSize);
    Assert(ThisMarker->OutputWriteCursor < SoundOutput->SecondaryBufferSize);
    Assert(ThisMarker->OutputLocation < SoundOutput->SecondaryBufferSize);
    Assert(ThisMarker->OutputByteCount < SoundOutput->SecondaryBufferSize);
    Assert(ThisMarker->FlipPlayCursor < SoundOutput->SecondaryBufferSize);
    Assert(ThisMarker->FlipWriteCursor < SoundOutput->SecondaryBufferSize);

    //white
    DWORD White = 0xFFFFFFFF;
    //red
    DWORD Red = 0xFFFF0000;
    //yellow
    DWORD Yellow = 0xFFFFFF00;
    //purple
    DWORD Purple = 0xFFFF00FF;

    int Top = PadY;
    int Bottom = PadY + LineHeight;
    if(MarkerIndex == CurrentMarkerIndex)
    {
      Top += LineHeight+PadY;
      Bottom += LineHeight+PadY;

      int FirstTop = Top;

      Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputPlayCursor, White);
      Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputWriteCursor, Red);

      Top += LineHeight+PadY;
      Bottom += LineHeight+PadY;

      Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation, White);
      Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation + ThisMarker->OutputByteCount, Red);

      Top += LineHeight+PadY;
      Bottom += LineHeight+PadY;

      Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, FirstTop, Bottom, ThisMarker->ExpectedFlipPlayCursor, Yellow);
    }

    Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor, White);
    Top += LineHeight + PadY;
    Bottom += LineHeight + PadY;

    Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor + 480*SoundOutput->BytesPerSample, Purple);
    Top += LineHeight + PadY;
    Bottom += LineHeight + PadY;

    Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipWriteCursor, Red);
  }
}
#endif

internal LARGE_INTEGER
Win32GetCurrentCounter()
{
  LARGE_INTEGER Result;
  QueryPerformanceCounter(&Result);
  return Result;
}

internal real32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
  real32 Result = ((float)(End.QuadPart - Start.QuadPart)) / GlobalPerfCounterFrequency;
  return Result;
}

internal real32
Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZone)
{
  real32 Result = 0;
  if(Value < -DeadZone)
  {
    Result = (float)((Value + DeadZone) / (32768 - DeadZone));
  }
  else if(Value > DeadZone)
  {
    Result = (float)((Value - DeadZone) / (32767 - DeadZone));
  }
  return Result;
}


DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
  VirtualFree(Memory, 0, MEM_RELEASE);
}


DEBUG_PLATFORM_READ_FILE(DEBUGPlatformReadFile)
{
  debug_read_file_result Result = {};
  HANDLE FileHandle = CreateFile(Filename, GENERIC_READ,
                                  FILE_SHARE_READ,
                                  0,
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,
                                  0);
  if(FileHandle !=  INVALID_HANDLE_VALUE)
  {
    LARGE_INTEGER FileSize;
    if(GetFileSizeEx(FileHandle, &FileSize))
    {
      Result.ContentSize = SafeTruncateUint64(FileSize.QuadPart);
      Result.Content = VirtualAlloc(0, Result.ContentSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
      if(Result.Content)
      {
        DWORD BytesRead;
        if(ReadFile(FileHandle, Result.Content, Result.ContentSize, &BytesRead, 0) && BytesRead == Result.ContentSize)
        {
        }
        else
        {
          DEBUGPlatformFreeFileMemory(Thread, Result.Content);
          Result.Content = 0;
        }
      }
      else
      {

      }
    }
    else
    {
    }
    CloseHandle(FileHandle);
  }
  else
  {
    //TODO: logging
  }
  return Result;
}

DEBUG_PLATFORM_WRITE_FILE(DEBUGPlatformWriteFile)
{
  bool Result = false;
  HANDLE FileHandle = CreateFile(Filename, GENERIC_WRITE,
                                  0,
                                  0,
                                  CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  0);
  if(FileHandle != INVALID_HANDLE_VALUE)
  {
    DWORD BytesWritten;
    if(WriteFile(FileHandle, Content, Size, &BytesWritten, 0))
    {
      Result = (BytesWritten == Size);
    }
    else
    {

    }
    CloseHandle(FileHandle);
  }
  else
  {
    //TODO: logging
  }
  return Result;
}


internal void
Win32LoadXInput(void)
{
  HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
  if(XInputLibrary){
    XInputGetState_ = (x_input_get_state *) GetProcAddress(XInputLibrary, "XInputGetState");
    XInputSetState_ = (x_input_set_state *) GetProcAddress(XInputLibrary, "XInputSetState");
  }
}

internal void
Win32ProcessXInputButton(DWORD XInputButtonState,
                          game_button_state *NewState,
                          game_button_state *OldState,
                          DWORD ButtonBit)
{
  NewState->EndedDown = (XInputButtonState & ButtonBit) == 1;
  NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}


internal void
Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
  HMODULE DSouondLibrary = LoadLibraryA("dsound.dll");
  if(DSouondLibrary)
  {
    direct_sound_create *DirectSoundCreate = (direct_sound_create *)
      GetProcAddress(DSouondLibrary, "DirectSoundCreate");

    LPDIRECTSOUND DirectSound;
    if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound,0)))
    {
      WAVEFORMATEX WaveFormat = {};
      WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
      WaveFormat.nChannels = 2;
      WaveFormat.nSamplesPerSec = SamplesPerSecond;
      WaveFormat.wBitsPerSample = 16;
      WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8 ;
      WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
      WaveFormat.cbSize = 0;

      if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
      {
        DSBUFFERDESC BufferDesc = {};
        BufferDesc.dwSize = sizeof(BufferDesc);
        BufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;

        LPDIRECTSOUNDBUFFER PrimaryBuffer;
        if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDesc, &PrimaryBuffer, 0)))
        {
          HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);
          if(SUCCEEDED(Error)){
            OutputDebugStringA("Primary buffer format was set");
          }
          else{

          }
        }
        else{

        }
      }
      else{

      }

      DSBUFFERDESC BufferDesc = {};
      BufferDesc.dwSize = sizeof(BufferDesc);
      BufferDesc.dwFlags = 0;
      BufferDesc.dwBufferBytes = BufferSize;
      BufferDesc.lpwfxFormat = &WaveFormat;
      HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDesc, &GlobalSecondaryBuffer, 0);
      if(SUCCEEDED(Error)){
        OutputDebugStringA("DirectSound secondary buffer created successfully");
      }
      else{

      }
    }
    else{

    }
  }
  else{

  }
}

win32_window_dimension
Win32GetWindowDimension(HWND Window){
  win32_window_dimension Result;

  RECT ClientRect;
  GetClientRect(Window, &ClientRect);
  Result.Width = ClientRect.right - ClientRect.left;
  Result.Height = ClientRect.bottom - ClientRect.top;
  return Result;
}

internal void Win32DisplayBufferInWindow(HDC DeviceContext,int WindowWidth, int WindowHeight,
  win32_offscreen_buffer Buffer)
{
  StretchDIBits(DeviceContext,
    0, 0, Buffer.Width, Buffer.Height,
    0, 0, Buffer.Width, Buffer.Height,
    Buffer.Memory,
    &Buffer.Info,
    DIB_RGB_COLORS, SRCCOPY);
}

internal void
Win32ProcessKeyboardMessage(game_button_state *ButtonState, bool IsDown)
{
  if(ButtonState->EndedDown != IsDown)
  {
    ButtonState->EndedDown = IsDown;
    ButtonState->HalfTransitionCount++;
  }
}


internal void
Win32ProcessPendingMessages(win32_state *Win32State, game_controller_input *KeyboardController)
{
  MSG Message;
  while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
  {
    switch(Message.message)
    {
      case WM_QUIT:
      {
        GlobalRunning = false;
      } break;

      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP:
      case WM_KEYDOWN:
      case WM_KEYUP:
      {
        uint32 VKCode = (uint32)Message.wParam;
        bool WasDown = ((Message.lParam & (1 << 30)) != 0);
        bool IsDown = ((Message.lParam & (1<< 31)) == 0 );
        if(WasDown != IsDown)
        {
          switch(VKCode){
            case 'X':
            {
              if(IsDown)
              {
                GlobalPause = !GlobalPause;
              }
            } break;
            case 'W':
            {
              Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
            } break;
            case 'A':
            {
              Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
            } break;
            case 'S':
            {
              Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
            } break;
            case 'D':
            {
              Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
            } break;
            case 'Q':
            {
              Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
            } break;
            case 'E':
            {
              Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
            } break;
            case 'R':
            {
              if(IsDown)
              {
                if(Win32State->InputRecordingIndex == 0)
                {
                  if(Win32State->InputPlayingbackIndex == 0)
                  {
                    Win32BeginRecordingInput(Win32State, 1);
                  }
                  else
                  {
                    Win32EndInputPlayingback(Win32State);
                  }
                }
                else
                {
                  Win32EndRecordingInput(Win32State);
                  Win32BeginInputPlayingback(Win32State, 1);
                }
              }
            } break;
            case VK_UP:
            {
              Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
            } break;
            case VK_DOWN:
            {
              Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
            } break;
            case VK_LEFT:
            {
              Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
            } break;
            case VK_RIGHT:
            {
              Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
            } break;
            case VK_ESCAPE:
            {
              Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
            } break;
            case VK_SPACE:
            {
              Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
            } break;
          }
        }
      } break;

      default:
      {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
      } break;
    }
  }
}


internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
  if(Buffer->Memory){
    VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
  }
  Buffer->Width = Width;
  Buffer->Height = Height;

  Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
  Buffer->Info.bmiHeader.biWidth = Width;
  Buffer->Info.bmiHeader.biHeight = -Height;
  Buffer->Info.bmiHeader.biPlanes = 1;
  Buffer->Info.bmiHeader.biBitCount = 32;
  Buffer->Info.bmiHeader.biCompression = BI_RGB;

  int BytesPerPixel = 4;
  int BitmapMemorysize = Width * Height * BytesPerPixel;
  Buffer->Memory = VirtualAlloc(0, BitmapMemorysize, MEM_COMMIT, PAGE_READWRITE);
  Buffer->Pitch = BytesPerPixel * Width;
  Buffer->BytesPerPixel = BytesPerPixel;
}

LRESULT CALLBACK MainWindowCallback(HWND Window,
                            UINT Message,
                            WPARAM wParam,
                            LPARAM lParam)
{
  LRESULT Result = 0;
  switch(Message){
    case WM_DESTROY:
    {
      GlobalRunning = false;
    } break;

    case WM_CLOSE:
    {
      GlobalRunning = false;
    } break;

    case WM_ACTIVATEAPP:
    {
      OutputDebugStringA("ACTIVATEAPP\n");
    } break;

    case WM_PAINT:
    {
      OutputDebugStringA("WM Paint");
      PAINTSTRUCT Paint;
      HDC DeviceContext = BeginPaint(Window,&Paint);
      win32_window_dimension Dimension = Win32GetWindowDimension(Window);
      Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackbuffer);
      EndPaint(Window,&Paint);
    } break;

    default:
    {
      Result = DefWindowProc(Window,Message,wParam,lParam);
    }
  }

  return Result;
}

internal void
Win32ClearSoundBuffer(win32_sound_output *SoundOutput)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;

    if(SUCCEEDED(GlobalSecondaryBuffer->Lock(0,
                                            SoundOutput->SecondaryBufferSize,
                                            &Region1, &Region1Size,
                                            &Region2, &Region2Size,
                                            0))){
      int8 *ByteOut = (int8 *)Region1;
      for(DWORD ByteIndex = 0;
        ByteIndex < Region1Size;
        ByteIndex++)
      {
          *ByteOut++ = 0;
      }

      ByteOut = (int8 *)Region2;
      for(DWORD ByteIndex = 0;
        ByteIndex < Region2Size;
        ByteIndex++)
      {
          *ByteOut++ = 0;
      }
      GlobalSecondaryBuffer->Unlock(Region1,Region1Size,Region2,Region2Size);
    }
}

internal void
Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite, game_sound_buffer *SourceBuffer)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;

    if(SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock,
                                            BytesToWrite,
                                             &Region1, &Region1Size,
                                             &Region2, &Region2Size,
                                             0))){
        DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
        int16 *SampleOut = (int16 *)Region1;
        int16 *Source = SourceBuffer->Samples;
        for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; SampleIndex++){
            *SampleOut++ = *Source++;
            *SampleOut++ = *Source++;
            ++SoundOutput->RunningSampleIndex;
        }

        DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
        SampleOut = (int16 *)Region2;
        for(DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; SampleIndex++){
            *SampleOut++ = *Source++;
            *SampleOut++ = *Source++;
            ++SoundOutput->RunningSampleIndex;
        }

        GlobalSecondaryBuffer->Unlock(Region1,Region1Size,Region2,Region2Size);
    }
}


int CALLBACK
WinMain(HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR CmdLine,
    int CmdShow)
{
  thread_context Context = {};
  win32_state Win32State = {};

  DWORD EXEPathLength = GetModuleFileNameA(0, Win32State.EXEPath, sizeof(Win32State.EXEPath));
  Win32State.OnePastLastEXEPathSlash = Win32State.EXEPath;
  for(char *Scan = Win32State.EXEPath;
    *Scan;
    Scan++)
  {
    if(*Scan == '\\')
    {
      Win32State.OnePastLastEXEPathSlash = Scan + 1;
    }
  }

  char SourceGameCodeDLLFileName[] = "handmade.dll";
  char SourceGameCodeDLLFullPath[MAX_PATH];
  CatStrings(Win32State.OnePastLastEXEPathSlash - Win32State.EXEPath,
    Win32State.EXEPath,
    sizeof(SourceGameCodeDLLFileName) - 1,
    SourceGameCodeDLLFileName,
    SourceGameCodeDLLFullPath
  );

  char TempGameCodeDLLFileName[] = "handmade_temp.dll";
  char TempGameCodeDLLFullPath[MAX_PATH];
  CatStrings(Win32State.OnePastLastEXEPathSlash - Win32State.EXEPath,
    Win32State.EXEPath,
    sizeof(TempGameCodeDLLFileName) - 1,
    TempGameCodeDLLFileName,
    TempGameCodeDLLFullPath
  );

  UINT DesiredSchedulerMS = 1;
  bool SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);





  LARGE_INTEGER PerfCountFrequencyResult;
  QueryPerformanceFrequency(&PerfCountFrequencyResult);
  GlobalPerfCounterFrequency = PerfCountFrequencyResult.QuadPart;

  WNDCLASS WindowClass = {};
  char *GameSource = "handmade.dll";
  Win32LoadXInput();

  //TODO: check if owndc,hredraw,vredraw matter
  WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
  WindowClass.lpfnWndProc = MainWindowCallback;
  WindowClass.hInstance = Instance;
  // HICON     hIcon;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";

  if(RegisterClass(&WindowClass))
  {
    HWND Window =  CreateWindowEx(
      0,
      WindowClass.lpszClassName,
      "HandmadeHero",
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
    if(Window){
      HDC DeviceContext = GetDC(Window);

      int MonitorRefreshHz = 60;
      int Win32RefreshRate = GetDeviceCaps(DeviceContext, VREFRESH);
      if(Win32RefreshRate > 1)
      {
          MonitorRefreshHz = Win32RefreshRate;
      }

      int GameUpdateHz = MonitorRefreshHz / 2;
      real32 TargetSecondsPerFrame = 1.0f / GameUpdateHz;

      Win32ResizeDIBSection(&GlobalBackbuffer, 960, 540);

      game_memory GameMemory = {};
      GameMemory.PermanentStorageSize = Megabytes(64);
      GameMemory.TransientStorageSize = Megabytes(10);
      GameMemory.DEBUGPlatformWriteFile = DEBUGPlatformWriteFile;
      GameMemory.DEBUGPlatformReadFile = DEBUGPlatformReadFile;
      GameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;

#if HANDMADE_DEBUG
      LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
      LPVOID BaseAddress = 0;
#endif


      Win32State.TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;

      GameMemory.PermanentStorage = (uint8 *)VirtualAlloc(BaseAddress, Win32State.TotalSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
      Win32State.GameMemoryBlock = GameMemory.PermanentStorage;
      GameMemory.TransientStorage = (uint8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize;


      //Win32 DirectSound
      win32_sound_output SoundOutput = {};
      SoundOutput.SamplesPerSecond = 48000;
      SoundOutput.BytesPerSample = sizeof(int16) * 2;
      SoundOutput.SecondaryBufferSize = SoundOutput.BytesPerSample * SoundOutput.SamplesPerSecond;
      SoundOutput.LatencySampleCount = 3 * (SoundOutput.SamplesPerSecond / GameUpdateHz);
      SoundOutput.SafetyBytes = (SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample / GameUpdateHz) / 3;

      Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
      Win32ClearSoundBuffer(&SoundOutput);
      GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

      int16 *Samples = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_COMMIT, PAGE_READWRITE);

      GlobalRunning = true;

#if 0
      while(GlobalRunning)
      {
        DWORD PlayCursor;
        DWORD WriteCursor;
        GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);
        char TextBuffer[256];
        _snprintf_s(TextBuffer, sizeof(TextBuffer), "PC:%u WC:%u\n", PlayCursor, WriteCursor);
        OutputDebugStringA(TextBuffer);
      }
#endif

      if(Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
      {

        //Init Input
        game_input Input[2] = {};
        game_input *NewInput = &Input[0];
        game_input *OldInput = &Input[1];

        //For time measure
        LARGE_INTEGER LastCounter = Win32GetCurrentCounter();
        LARGE_INTEGER FlipClock = Win32GetCurrentCounter();
        uint64 LastCycleCounter = __rdtsc();

        int DebugTimeMarkerIndex = 0;
        win32_debug_time_marker DebugTimeMarkers[30] = {0};

        DWORD AudioLantencyBytes = 0;
        real32 AudioLantencySeconds = 0;
        bool SoundIsValid = false;

        win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath);


        while(GlobalRunning)
        {
          NewInput->TimeForFrame = TargetSecondsPerFrame;
          FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
          if(CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0)
          {
            Win32UnloadGameCode(&Game);
            Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath);
          }

          game_controller_input *OldKeyboardController = &OldInput->Controllers[0];
          game_controller_input *NewKeyboardController = &NewInput->Controllers[0];
          *NewKeyboardController = {0};
          NewKeyboardController->IsConnected = true;

          POINT MouseP;
          GetCursorPos(&MouseP);
          ScreenToClient(Window, &MouseP);
          NewInput->MouseX = MouseP.x;
          NewInput->MouseY = MouseP.y;
          NewInput->MouseZ = 0; // TODO(casey): Support mousewheel?
          Win32ProcessKeyboardMessage(&NewInput->MouseButtons[0],
                                      GetKeyState(VK_LBUTTON) & (1 << 15));
          Win32ProcessKeyboardMessage(&NewInput->MouseButtons[1],
                                      GetKeyState(VK_MBUTTON) & (1 << 15));
          Win32ProcessKeyboardMessage(&NewInput->MouseButtons[2],
                                      GetKeyState(VK_RBUTTON) & (1 << 15));
          Win32ProcessKeyboardMessage(&NewInput->MouseButtons[3],
                                      GetKeyState(VK_XBUTTON1) & (1 << 15));
          Win32ProcessKeyboardMessage(&NewInput->MouseButtons[4],
                                      GetKeyState(VK_XBUTTON2) & (1 << 15));

          for(int ButtonIndex = 0;
            ButtonIndex < ArrayCount(NewKeyboardController->Buttons);
            ButtonIndex++)
          {
            NewKeyboardController->Buttons[ButtonIndex].EndedDown =
              OldKeyboardController->Buttons[ButtonIndex].EndedDown;
          }

          Win32ProcessPendingMessages(&Win32State, NewKeyboardController);

          if(GlobalPause)
          {
            continue;
          }

          uint32 MaxControllerCount = XUSER_MAX_COUNT;
          if(MaxControllerCount > (ArrayCount(NewInput->Controllers) - 1))
          {
            MaxControllerCount = ArrayCount(NewInput->Controllers) - 1;
          }
          // Pull message from controller
          for(DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ControllerIndex++)
          {
            DWORD InputIndex = ControllerIndex + 1;
            XINPUT_STATE ControllerState;

            game_controller_input *NewController = &NewInput->Controllers[InputIndex];
            game_controller_input *OldController = &OldInput->Controllers[InputIndex];

            if(XInputGetState_(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
            {
              NewController->IsConnected = true;
              NewController->IsAnalog = OldController->IsAnalog;
              XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

              NewController->StickAverageX = Win32ProcessXInputStickValue(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
              NewController->StickAverageY = Win32ProcessXInputStickValue(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

              if(NewController->StickAverageY != 0.0f || NewController->StickAverageX != 0.0f)
              {
                NewController->IsAnalog = true;
              }

              if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
              {
                NewController->StickAverageY = 1.0f;
                NewController->IsAnalog = false;
              }
              if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
              {
                NewController->StickAverageY = -1.0f;
                NewController->IsAnalog = false;
              }
              if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
              {
                NewController->StickAverageX = -1.0f;
                NewController->IsAnalog = false;
              }
              if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
              {
                NewController->StickAverageX = 1.0f;
                NewController->IsAnalog = false;
              }

              float Threshold = 0.5f;
              Win32ProcessXInputButton(
                (NewController->StickAverageY > Threshold) ? 1 : 0,
                                        &NewController->MoveUp,
                                        &OldController->MoveUp,
                                        1);
              Win32ProcessXInputButton(
                (NewController->StickAverageY < -Threshold) ? 1 : 0,
                                        &NewController->MoveDown,
                                        &OldController->MoveDown,
                                        1);
              Win32ProcessXInputButton(
                (NewController->StickAverageX > Threshold) ? 1 : 0,
                                        &NewController->MoveRight,
                                        &OldController->MoveRight,
                                        1);
              Win32ProcessXInputButton(
                (NewController->StickAverageX < -Threshold) ? 1 : 0,
                                        &NewController->MoveLeft,
                                        &OldController->MoveLeft,
                                        1);


              Win32ProcessXInputButton(Pad->wButtons,
                                        &NewController->ActionDown,
                                        &OldController->ActionDown,
                                        XINPUT_GAMEPAD_A);
              Win32ProcessXInputButton(Pad->wButtons,
                                        &NewController->ActionRight,
                                        &OldController->ActionRight,
                                        XINPUT_GAMEPAD_B);
              Win32ProcessXInputButton(Pad->wButtons,
                                        &NewController->ActionLeft,
                                        &OldController->ActionLeft,
                                        XINPUT_GAMEPAD_X);
              Win32ProcessXInputButton(Pad->wButtons,
                                        &NewController->ActionUp,
                                        &OldController->ActionUp,
                                        XINPUT_GAMEPAD_Y);
              Win32ProcessXInputButton(Pad->wButtons,
                                        &NewController->LeftShoulder,
                                        &OldController->LeftShoulder,
                                        XINPUT_GAMEPAD_LEFT_SHOULDER);
              Win32ProcessXInputButton(Pad->wButtons,
                                        &NewController->RightShoulder,
                                        &OldController->RightShoulder,
                                        XINPUT_GAMEPAD_RIGHT_SHOULDER);
              Win32ProcessXInputButton(Pad->wButtons,
                                        &NewController->Start,
                                        &OldController->Start,
                                        XINPUT_GAMEPAD_START);
              Win32ProcessXInputButton(Pad->wButtons,
                                        &NewController->Back,
                                        &OldController->Back,
                                        XINPUT_GAMEPAD_BACK);
            }
            else{
              NewController->IsConnected = false;
            }
          }

           //Game Graphics Buffer
          game_offscreen_buffer Buffer = {};
          Buffer.Memory = GlobalBackbuffer.Memory;
          Buffer.Width = GlobalBackbuffer.Width;
          Buffer.Height = GlobalBackbuffer.Height;
          Buffer.Pitch = GlobalBackbuffer.Pitch;
          Buffer.BytesPerPixel = GlobalBackbuffer.BytesPerPixel;

          if(Win32State.InputRecordingIndex)
          {
            Win32RecordInput(&Win32State, NewInput);
          }

          if(Win32State.InputPlayingbackIndex)
          {
            Win32PlaybackInput(&Win32State, NewInput);
          }

          Game.GameUpdateVideo(&Context, &GameMemory, NewInput, &Buffer);

          LARGE_INTEGER AudioClock = Win32GetCurrentCounter();
          real32 FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipClock, AudioClock);

          // DirectSound test
          DWORD PlayCursor = 0 ;
          DWORD WriteCursor = 0 ;
          if(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
          {
            if(!SoundIsValid)
            {
              SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
              SoundIsValid = true;
            }
            DWORD BytesToWrite = 0 ;
            DWORD ByteToLock = 0 ;

            DWORD ExpectedSoundBytesPerFrame = (SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample) / GameUpdateHz;
            real32 SecondsLeftUntilFlip = (TargetSecondsPerFrame - FromBeginToAudioSeconds);
            DWORD ExpectedBytesUntilFlip = (DWORD)((SecondsLeftUntilFlip / TargetSecondsPerFrame) * ExpectedSoundBytesPerFrame);

            DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedBytesUntilFlip;

            DWORD SafeWriteCursor = WriteCursor;
            if(SafeWriteCursor < PlayCursor)
            {
              SafeWriteCursor += SoundOutput.SecondaryBufferSize;
            }
            Assert(SafeWriteCursor > PlayCursor);
            SafeWriteCursor += SoundOutput.SafetyBytes;

            bool AudioIsLowLatency = (SafeWriteCursor < ExpectedFrameBoundaryByte);

            DWORD TargetCursor = 0;
            if(AudioIsLowLatency)
            {
              TargetCursor = ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame;
            }
            else
            {
              TargetCursor = SafeWriteCursor + ExpectedSoundBytesPerFrame;
            }
            TargetCursor = TargetCursor % SoundOutput.SecondaryBufferSize;

            ByteToLock = ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize);

            if(ByteToLock > TargetCursor)
            {
              BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
              BytesToWrite += TargetCursor;
            }
            else
            {
              BytesToWrite = TargetCursor - ByteToLock;
            }

            // Game Sound Buffer
            game_sound_buffer SoundBuffer = {};
            SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
            SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
            SoundBuffer.Samples = Samples;
            Game.GameUpdateAudio(&Context, &GameMemory, &SoundBuffer);

#if 0
            win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
            Marker->OutputPlayCursor = PlayCursor;
            Marker->OutputWriteCursor = WriteCursor;
            Marker->OutputLocation = ByteToLock;
            Marker->OutputByteCount = BytesToWrite;
            Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte;

            DWORD UnwrappedWriteCursor = WriteCursor;
            if(UnwrappedWriteCursor < PlayCursor)
            {
              UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
            }
            AudioLantencyBytes = UnwrappedWriteCursor - PlayCursor;
            AudioLantencySeconds = (AudioLantencyBytes / SoundOutput.BytesPerSample) / (real32)SoundOutput.SamplesPerSecond;

            char TextBuffer[256];
            _snprintf_s(TextBuffer, sizeof(TextBuffer),
              "BTL:%u TC:%u BTW:%u - PC:%u WC:%u DELTA:%u (%fs)\n",
              ByteToLock, TargetCursor, BytesToWrite, PlayCursor, WriteCursor, AudioLantencyBytes, AudioLantencySeconds);
            OutputDebugStringA(TextBuffer);
#endif

            Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
          }
          else
          {
            SoundIsValid = false;
          }

          // For time measure
          float SecondsElapsedSoFar = Win32GetSecondsElapsed(LastCounter, Win32GetCurrentCounter());
          if(SecondsElapsedSoFar < TargetSecondsPerFrame)
          {
            if(SleepIsGranular)
            {
              DWORD SleepMS = (DWORD)(1000 * (TargetSecondsPerFrame - SecondsElapsedSoFar));
              Sleep(SleepMS);
            }
            SecondsElapsedSoFar = Win32GetSecondsElapsed(LastCounter, Win32GetCurrentCounter());
            while(SecondsElapsedSoFar < TargetSecondsPerFrame)
            {
              SecondsElapsedSoFar = Win32GetSecondsElapsed(LastCounter, Win32GetCurrentCounter());
            }
          }
          else
          {
            //TODO: We are missing rate here!
            //TODO: Logging
          }

          uint64 EndCycleCounter = __rdtsc();
          LARGE_INTEGER EndCounter = Win32GetCurrentCounter();
          uint64 CyclesElapsed = EndCycleCounter - LastCycleCounter;
          int64  CounterElapsed = EndCounter.QuadPart   - LastCounter.QuadPart;

          LastCounter = EndCounter;
          LastCycleCounter = EndCycleCounter;

#if 0
          Win32DebugSyncDisplay(&GlobalBackbuffer, ArrayCount(DebugTimeMarkers), DebugTimeMarkers, DebugTimeMarkerIndex - 1, &SoundOutput, TargetSecondsPerFrame);
#endif


          win32_window_dimension Dimension = Win32GetWindowDimension(Window);
          Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackbuffer);

          FlipClock = Win32GetCurrentCounter();

#if HANDMADE_DEBUG
          {
            DWORD PlayCursor = 0;
            DWORD WriteCursor = 0;
            if(GlobalSecondaryBuffer -> GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
            {
              Assert(DebugTimeMarkerIndex < ArrayCount(DebugTimeMarkers));
              win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
              Marker->FlipPlayCursor = PlayCursor;
              Marker->FlipWriteCursor = WriteCursor;
            }
          }


#endif

          //Siwtch the input
          game_input *Temp = NewInput;
          NewInput = OldInput;
          OldInput = Temp;


#if 0
          float MSPerFrame = CounterElapsed * 1000.0f  / GlobalPerfCounterFrequency ;
          float FPS = (float)GlobalPerfCounterFrequency / CounterElapsed ;
          float MCPF = (float)CyclesElapsed  / 1000 / 1000;
          char DebugBuffer[256];
          sprintf_s(DebugBuffer, 256, "%.2fms/f  %.2ff/s  %.2fMC/f \n", MSPerFrame, FPS, MCPF);
          OutputDebugStringA(DebugBuffer);
#endif

#if 0
          ++DebugTimeMarkerIndex;
          if(DebugTimeMarkerIndex == ArrayCount(DebugTimeMarkers))
          {
            DebugTimeMarkerIndex = 0;
          }
#endif
        }
      }
    }
    else
    {
      //TODO: logging
    }
  }
  else
  {
    //TODO logging
  }
  return 0;
}
