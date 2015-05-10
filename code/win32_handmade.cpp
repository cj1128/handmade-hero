#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>

#include "win32_handmade.h"
#include "handmade.cpp"


global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64_t GlobalPerfCounterFrequency;

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(f_x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable f_x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE(casey): XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(f_x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable f_x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal LARGE_INTEGER
Win32GetCurrentCounter()
{
  LARGE_INTEGER Result;
  QueryPerformanceCounter(&Result);
  return Result;
}

internal float
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
  float Result = ((float)(End.QuadPart - Start.QuadPart)) / GlobalPerfCounterFrequency;
  return Result;
}

internal float
Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZone)
{
  float Result = 0;
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

internal debug_read_file_result
DEBUGPlatformReadFile(char *Filename)
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
          DEBUGPlatformFreeFileMemory(Result.Content);
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

internal void
DEBUGPlatformFreeFileMemory(void *Memory)
{
  VirtualFree(Memory, 0, MEM_RELEASE);
}

internal bool
DEBUGPlatformWriteFile(char *Filename, uint32_t Size, void *Content)
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
Win32LoadXInput(void){
  HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
  if(XInputLibrary){
    XInputGetState = (f_x_input_get_state *) GetProcAddress(XInputLibrary, "XInputGetState");
    XInputSetState = (f_x_input_set_state *) GetProcAddress(XInputLibrary, "XInputSetState");
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
Win32InitDSound(HWND Window, int32_t SamplesPerSecond, int32_t BufferSize)
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
    0,0,WindowWidth,WindowHeight,
    0,0,Buffer.Width,Buffer.Height,
    Buffer.Memory,
    &Buffer.Info,
    DIB_RGB_COLORS,SRCCOPY);
}

internal void
Win32ProcessKeyboardMessage(game_button_state *ButtonState, bool IsDown)
{
  Assert(ButtonState->EndedDown != IsDown);
  ButtonState->EndedDown = IsDown;
  ButtonState->HalfTransitionCount++;
}


internal void
Win32ProcessPendingMessages(game_controller_input *KeyboardController)
{
  MSG Message;
  while( PeekMessage(&Message,0,0,0, PM_REMOVE) )
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
        uint32_t VKCode = (uint32_t)Message.wParam;
        bool WasDown = ((Message.lParam & (1 << 30)) != 0);
        bool IsDown = ((Message.lParam & (1<< 31)) == 0 );
        if(WasDown != IsDown)
        {
          switch(VKCode){
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
  Buffer->Info.bmiHeader.biHeight = Height;
  Buffer->Info.bmiHeader.biPlanes = 1;
  Buffer->Info.bmiHeader.biBitCount = 32;
  Buffer->Info.bmiHeader.biCompression = BI_RGB;

  int BytesPerPixel = 4;
  int BitmapMemorysize = Width * Height * BytesPerPixel;
  Buffer->Memory = VirtualAlloc(0, BitmapMemorysize, MEM_COMMIT, PAGE_READWRITE);
  Buffer->Pitch = BytesPerPixel * Width;
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
      int8_t *ByteOut = (int8_t *)Region1;
      for(DWORD ByteIndex = 0;
        ByteIndex < Region1Size;
        ByteIndex++)
      {
          *ByteOut++ = 0;
      }

      ByteOut = (int8_t *)Region2;
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
        int16_t *SampleOut = (int16_t *)Region1;
        int16_t *Source = SourceBuffer->Samples;
        for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; SampleIndex++){
            *SampleOut++ = *Source++;
            *SampleOut++ = *Source++;
            ++SoundOutput->RunningSampleIndex;
        }

        DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
        SampleOut = (int16_t *)Region2;
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

  UINT DesiredSchedulerMS = 1;
  bool SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

  //TODO: How do we reliably query on this on windows?
  int MonitorRefreshHz = 60;
  int GameUpdateHz = MonitorRefreshHz / 2;
  float TargetSecondsPerFrame = 1.0f / GameUpdateHz;


  LARGE_INTEGER PerfCountFrequencyResult;
  QueryPerformanceFrequency(&PerfCountFrequencyResult);
  GlobalPerfCounterFrequency = PerfCountFrequencyResult.QuadPart;

  WNDCLASS WindowClass = {};
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
      Win32ResizeDIBSection(&GlobalBackbuffer, 1920, 1080);

      game_memory GameMemory = {};
      GameMemory.PermanentStorageSize = Megabytes(64);
      GameMemory.TransientStorageSize = Gigabytes(1);

#if HANDMADE_DEBUG
      LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
      LPVOID BaseAddress = 0;
#endif

      uint64_t TotalSize = GameMemory.PermanentStorageSize +
        GameMemory.TransientStorageSize;
      GameMemory.PermanentStorage = (uint8_t *)VirtualAlloc(BaseAddress, TotalSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
      GameMemory.TransientStorage = (uint8_t *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize;


      //Win32 DirectSound
      win32_sound_output SoundOutput = {};
      SoundOutput.SamplesPerSecond = 48000;
      SoundOutput.BytesPerSample = sizeof(int16_t) * 2;
      SoundOutput.SecondaryBufferSize = SoundOutput.BytesPerSample * SoundOutput.SamplesPerSecond;
      SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;

      Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
      Win32ClearSoundBuffer(&SoundOutput);
      GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

      int16_t *Samples = (int16_t *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_COMMIT, PAGE_READWRITE);

      GlobalRunning = true;

      if(Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
      {

        //Init Input
        game_input Input[2] = {};
        game_input *NewInput = &Input[0];
        game_input *OldInput = &Input[1];

        //For time measure
        LARGE_INTEGER LastCounter;
        QueryPerformanceCounter(&LastCounter);
        uint64_t LastCycleCounter = __rdtsc();


        while(GlobalRunning)
        {
          game_controller_input *OldKeyboardController = &OldInput->Controllers[0];
          game_controller_input *NewKeyboardController = &NewInput->Controllers[0];
          *NewKeyboardController = {};

          for(int ButtonIndex = 0;
            ButtonIndex < ArrayCount(NewKeyboardController->Buttons);
            ButtonIndex++)
          {
            NewKeyboardController->Buttons[ButtonIndex].EndedDown =
              OldKeyboardController->Buttons[ButtonIndex].EndedDown;
          }

          Win32ProcessPendingMessages(NewKeyboardController);

          uint32_t MaxControllerCount = XUSER_MAX_COUNT;
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

            if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
            {
              NewController->IsConnected = true;
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

          // DirectSound test
          DWORD PlayCursor = 0 ;
          DWORD WriteCursor = 0 ;
          DWORD BytesToWrite = 0 ;
          DWORD ByteToLock = 0 ;
          DWORD TargetCursor = 0 ;
          bool SoundIsValid = false;

          if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor,&WriteCursor)))
          {
            ByteToLock = ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize);
            TargetCursor =
                  ((PlayCursor +
                    (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) %
                   SoundOutput.SecondaryBufferSize);
            if(ByteToLock > TargetCursor)
            {
              BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
              BytesToWrite += TargetCursor;
            }
            else
            {
              BytesToWrite = TargetCursor - ByteToLock;
            }
            SoundIsValid = true;
          }

          // Game Sound Buffer
          game_sound_buffer SoundBuffer = {};
          SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
          SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
          SoundBuffer.Samples = Samples;


          //Game Graphics Buffer
          game_offscreen_buffer Buffer = {};
          Buffer.Memory = GlobalBackbuffer.Memory;
          Buffer.Width = GlobalBackbuffer.Width;
          Buffer.Height = GlobalBackbuffer.Height;
          Buffer.Pitch = GlobalBackbuffer.Pitch;

          GameUpdateAndRender(&GameMemory, NewInput, &Buffer, &SoundBuffer);

          if(SoundIsValid){
            Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
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


          win32_window_dimension Dimension = Win32GetWindowDimension(Window);
          Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackbuffer);

          //Siwtch the input
          game_input *Temp = NewInput;
          NewInput = OldInput;
          OldInput = Temp;


          uint64_t EndCycleCounter = __rdtsc();
          LARGE_INTEGER EndCounter = Win32GetCurrentCounter();

          uint64_t CyclesElapsed = EndCycleCounter - LastCycleCounter;
          int64_t  CounterElapsed = EndCounter.QuadPart   - LastCounter.QuadPart;

#if 1
          float MSPerFrame = CounterElapsed * 1000.0f  / GlobalPerfCounterFrequency ;
          float FPS = (float)GlobalPerfCounterFrequency / CounterElapsed ;
          float MCPF = (float)CyclesElapsed  / 1000 / 1000;
          char DebugBuffer[256];
          sprintf_s(DebugBuffer, 256, "%.2fms/f  %.2ff/s  %.2fMC/f \n", MSPerFrame, FPS, MCPF);
          OutputDebugStringA(DebugBuffer);
#endif

          LastCounter  = EndCounter;
          LastCycleCounter = EndCycleCounter;


        }
      }
      else
      {
        //TODO logging
      }
    }
    else
    {
      //TODO: logging
    }
  }
  else{
    //TODO logging
  }

  return 0;
}


