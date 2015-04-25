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
global_variable int ToneHz = 256;

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
Win32InitDSound(HWND Window, int32_t SamplesPerSecond, int32_t BufferSize){
  HMODULE DSouondLibrary = LoadLibraryA("dsound.dll");
  if(DSouondLibrary){
    direct_sound_create *DirectSoundCreate = (direct_sound_create *)
      GetProcAddress(DSouondLibrary, "DirectSoundCreate");

    LPDIRECTSOUND DirectSound;
    if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound,0))){
      WAVEFORMATEX WaveFormat = {};
      WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
      WaveFormat.nChannels = 2;
      WaveFormat.nSamplesPerSec = SamplesPerSecond;
      WaveFormat.wBitsPerSample = 16;
      WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8 ;
      WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
      WaveFormat.cbSize = 0;

      if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))){
        DSBUFFERDESC BufferDesc = {};
        BufferDesc.dwSize = sizeof(BufferDesc);
        BufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;

        LPDIRECTSOUNDBUFFER PrimaryBuffer;
        if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDesc, &PrimaryBuffer, 0))){
          HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);
          if(SUCCEEDED(Error)){
            OutputDebugStringA("DirectSound set format successfully");
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
        OutputDebugStringA("allocate secondary buffer successfully");
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

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
      uint32_t VKCode = wParam;
      bool WasDown = ((lParam & (1 << 30)) != 0);
      bool IsDown = ((lParam & (1<< 31)) == 0 );
      switch(VKCode){
        case 'W':
        {
          ToneHz += 20;
          OutputDebugStringA("W");
        } break;
        case 'D':
        {
          OutputDebugStringA("D");
        } break;
      }
    }

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
      DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
      int16_t *SampleOut = (int16_t *)Region1;
      for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; SampleIndex++){
          *SampleOut++ = 0;
          *SampleOut++ = 0;
          ++SoundOutput->RunningSampleIndex;
      }
      DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
      SampleOut = (int16_t *)Region2;
      for(DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; SampleIndex++){
          *SampleOut++ = 0;
          *SampleOut++ = 0;
          ++SoundOutput->RunningSampleIndex;
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


  LARGE_INTEGER PerfCountFrequencyResult;
  QueryPerformanceFrequency(&PerfCountFrequencyResult);
  int64_t PerfCountFrequency = PerfCountFrequencyResult.QuadPart;

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

      //Init Input
      game_input Input[2] = {};
      game_input *NewInput = &Input[0];
      game_input *OldInput = &Input[1];

      //Win32 DirectSound
      win32_sound_output SoundOutput = {};
      SoundOutput.SamplesPerSecond = 48000;
      SoundOutput.BytesPerSample = sizeof(int16_t) * 2;
      SoundOutput.SecondaryBufferSize = SoundOutput.BytesPerSample * SoundOutput.SamplesPerSecond;
      SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 12;

      Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
      Win32ClearSoundBuffer(&SoundOutput);
      GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

      int16_t *Samples = (int16_t *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_COMMIT, PAGE_READWRITE);

      // Game Sound Buffer
      game_sound_buffer SoundBuffer = {};
      SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
      SoundBuffer.Samples = Samples;


      GlobalRunning = true;

      int XOffset = 0;
      int YOffset = 0;

      HDC DeviceContext = GetDC(Window);
      Win32ResizeDIBSection(&GlobalBackbuffer, 1920, 1080);

      //For time measure
      LARGE_INTEGER LastCounter;
      QueryPerformanceCounter(&LastCounter);
      uint64_t LastCycleCounter = __rdtsc();

      while(GlobalRunning)
      {
        MSG Message;
        while( PeekMessage(&Message,Window,0,0, PM_REMOVE) )
        {
          if(Message.message == WM_QUIT){
            GlobalRunning = false;
          }
          TranslateMessage(&Message);
          DispatchMessage(&Message);
        }

        int MaxControllerCount = XUSER_MAX_COUNT;
        if(MaxControllerCount > ArrayCount(NewInput->Controllers))
        {
          MaxControllerCount = ArrayCount(NewInput->Controllers);
        }
        // Pull message from controller
        for(DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ControllerIndex++)
        {
          XINPUT_STATE ControllerState;
          game_controller_input *NewController = &NewInput->Controllers[ControllerIndex];
          game_controller_input *OldController = &OldInput->Controllers[ControllerIndex];
          if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS){
            XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
            bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
            bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
            bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
            bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
            bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
            bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
            bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
            bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);

            NewController->IsAnalog = true;
            NewController->StartX = OldController->EndX;
            NewController->StartY = OldController->EndY;

            float X;
            if(Pad->sThumbLX < 0)
            {
              X = Pad->sThumbLX / 32768.0f;
            }
            else
            {
              X = Pad->sThumbLX / 32767.0f;
            }
            float Y;
            if(Pad->sThumbLY < 0)
            {
              Y = Pad->sThumbLY / 32768.0f;
            }
            else
            {
              Y = Pad->sThumbLY / 32767.0f;
            }

            NewController->MinX = NewController->MaxX = NewController->EndX = X;
            NewController->MinY = NewController->MaxY = NewController->EndY = Y;

            Win32ProcessXInputButton(Pad->wButtons,
                                      &NewController->Down,
                                      &OldController->Down,
                                      XINPUT_GAMEPAD_A);
            Win32ProcessXInputButton(Pad->wButtons,
                                      &NewController->Right,
                                      &OldController->Right,
                                      XINPUT_GAMEPAD_B);
            Win32ProcessXInputButton(Pad->wButtons,
                                      &NewController->Left,
                                      &OldController->Left,
                                      XINPUT_GAMEPAD_X);
            Win32ProcessXInputButton(Pad->wButtons,
                                      &NewController->Up,
                                      &OldController->Up,
                                      XINPUT_GAMEPAD_Y);
            Win32ProcessXInputButton(Pad->wButtons,
                                      &NewController->LeftShoulder,
                                      &OldController->LeftShoulder,
                                      XINPUT_GAMEPAD_LEFT_SHOULDER);
            Win32ProcessXInputButton(Pad->wButtons,
                                      &NewController->RightShoulder,
                                      &OldController->RightShoulder,
                                      XINPUT_GAMEPAD_RIGHT_SHOULDER);
          }
          else{

          }
        }

        //Game Graphics Buffer
        game_offscreen_buffer Buffer = {};
        Buffer.Memory = GlobalBackbuffer.Memory;
        Buffer.Width = GlobalBackbuffer.Width;
        Buffer.Height = GlobalBackbuffer.Height;
        Buffer.Pitch = GlobalBackbuffer.Pitch;

        // DirectSound test
        DWORD PlayCursor;
        DWORD WriteCursor;
        DWORD BytesToWrite;
        DWORD ByteToLock;
        bool SoundIsValid = false;
        if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor,&WriteCursor))){
          SoundIsValid = true;
          ByteToLock = ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize);
          DWORD TargetCursor =
                ((PlayCursor +
                  (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) %
                 SoundOutput.SecondaryBufferSize);

          BytesToWrite;
          if(ByteToLock > TargetCursor){
              BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
              BytesToWrite += TargetCursor;
          }
          else{
              BytesToWrite = TargetCursor - ByteToLock;
          }
        }

        SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;

        GameUpdateAndRender(&GameMemory, NewInput, &Buffer, &SoundBuffer, ToneHz);

        if(SoundIsValid){
          Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
        }

        win32_window_dimension Dimension = Win32GetWindowDimension(Window);
        Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackbuffer);
        XOffset++;
        YOffset += 2;

        // For time measure
        uint64_t EndCycleCounter = __rdtsc();

        LARGE_INTEGER EndCounter;
        QueryPerformanceCounter(&EndCounter);
        uint64_t CyclesElapsed = EndCycleCounter - LastCycleCounter;
        int64_t  CounterElapsed = EndCounter.QuadPart   - LastCounter.QuadPart;
        float MSPerFrame = CounterElapsed * 1000.0f  / PerfCountFrequency ;
        float FPS = (float)PerfCountFrequency / CounterElapsed ;
        float MCPF = (float)CyclesElapsed  / 1000 / 1000;

        char DebugBuffer[256];
        sprintf(DebugBuffer, "%.2fms/f  %.2ff/s  %.2fMC/f \n", MSPerFrame, FPS, MCPF);
        OutputDebugStringA(DebugBuffer);

        LastCounter  = EndCounter;
        LastCycleCounter = EndCycleCounter;

      }
    }
    else{
      //TODO logging
    }
  }
  else{
    //TODO logging
  }

  return 0;
}


