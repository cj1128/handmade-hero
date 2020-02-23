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
global_variable x_input_get_state *xInputGetState_ = XInputGetStateStub;

XINPUT_SET_STATE(XInputSetStateStub) {
  return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *xInputSetState_ = XInputSetStateStub;

#define XInputGetState xInputGetState_
#define XInputSetState xInputSetState_

global_variable bool globalRunning = true;
global_variable bool globalDebugUpdateWindow = true;
global_variable win32_offscreen_buffer globalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER globalSoundBuffer;
global_variable uint64 globalPerfCounterFrequency;
global_variable bool32 globalShowCursor;

DEBUG_PLATFORM_READ_FILE(DebugPlatformReadFile) {
  debug_read_file_result result = {};

  HANDLE file = CreateFileA(
    fileName,
    GENERIC_READ,
    0,
    0,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    0
  );

  LARGE_INTEGER fileSize;

  if(file != INVALID_HANDLE_VALUE) {
    if(GetFileSizeEx(file, &fileSize)) {
      uint32 fileSize32 = SafeTruncateUInt64(fileSize.QuadPart);
      void *memory = VirtualAlloc(0, fileSize32, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
      if(memory) {
        DWORD bytesRead;
        if(ReadFile(file, memory, fileSize32, &bytesRead, 0) && bytesRead == fileSize32){
          result.size = fileSize32;
          result.memory = memory;
        } else {
          // TODO: logging
        }
      } else {
        // TODO: logging
      }
    } else {
      // TODO: logging
    }

    CloseHandle(file);
  } else {
    // TODO: logging
  }

  return result;
}

DEBUG_PLATFORM_WRITE_FILE(DebugPlatformWriteFile) {
  bool32 result = false;

  HANDLE file = CreateFileA(
    fileName,
    GENERIC_WRITE,
    0,
    0,
    CREATE_ALWAYS,
    FILE_ATTRIBUTE_NORMAL,
    0
  );

  if(file != INVALID_HANDLE_VALUE) {
    DWORD bytesWritten;
    if(WriteFile(file, memory, fileSize, &bytesWritten, 0)) {
      result = bytesWritten == fileSize;
    } else {
      // TODO: logging
    }

    CloseHandle(file);
  } else {
    // TODO: logging
  }

  return result;
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(DebugPlatformFreeFileMemory) {
  VirtualFree(memory, 0, MEM_RELEASE);
}

internal void
Win32BuildPathInExeDir(
  win32_state *state,
  char *src,
  int srcSize,
  char *dst,
  int dstSize
) {
  strncpy_s(dst, dstSize, state->exePath, state->exeDirLength);
  strncpy_s(dst + state->exeDirLength, dstSize - state->exeDirLength, src, srcSize);
}

internal void
Win32GetExePath(win32_state *state) {
  DWORD fileNameSize = GetModuleFileNameA(NULL, state->exePath, sizeof(state->exePath));
  char *onePastLastSlash = state->exePath;
  for(char *scan = state->exePath + fileNameSize; ; --scan) {
    if(*scan == '\\') {
      onePastLastSlash = scan + 1;
      break;
    }
  }
  state->exeDirLength = (int)(onePastLastSlash - state->exePath);
}

internal win32_replay_buffer *
Win32GetReplayBuffer(win32_state *state, int index) {
  return &state->replayBuffers[index - 1];
}

internal void
Win32GetReplayInputFilePath(
  win32_state *state,
  int index,
  char filePath[],
  int filePathSize
) {
  char fileName[30];
  sprintf_s(fileName, sizeof(fileName), "handmade_input_%d.hmi", index);
  Win32BuildPathInExeDir(
    state,
    fileName,
    int(strlen(fileName)),
    filePath,
    filePathSize
  );
}

internal void
Win32BeginInputRecording(win32_state *state, int index) {
  state->inputRecordingIndex = index;
  win32_replay_buffer *buffer = Win32GetReplayBuffer(state, index);

  char filePath[WIN32_MAX_PATH];
  Win32GetReplayInputFilePath(state, index, filePath, sizeof(filePath));
  buffer->inputHandle = CreateFileA(
    filePath,
    GENERIC_WRITE,
    0,
    0,
    CREATE_ALWAYS,
    FILE_ATTRIBUTE_NORMAL,
    0
  );

  CopyMemory(
    buffer->memory,
    state->gameMemory,
    state->memorySize
  );
}

global_variable WINDOWPLACEMENT globalWindowPlacement = {
  sizeof(globalWindowPlacement)
};
internal void
ToggleFullscreen(HWND window) {
  DWORD windowStyle = GetWindowLong(window, GWL_STYLE);

  if (windowStyle & WS_OVERLAPPEDWINDOW) {
    MONITORINFO monitorInfo = { sizeof(monitorInfo) };

    if (GetWindowPlacement(window, &globalWindowPlacement) &&
        GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &monitorInfo)) {
      SetWindowLong(window, GWL_STYLE, windowStyle & ~WS_OVERLAPPEDWINDOW);
      SetWindowPos(
        window, HWND_TOP,
        monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
        monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
        monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
        SWP_NOOWNERZORDER | SWP_FRAMECHANGED
      );
    }
  } else {
    SetWindowLong(window, GWL_STYLE,
                  windowStyle | WS_OVERLAPPEDWINDOW);
    SetWindowPlacement(window, &globalWindowPlacement);
    SetWindowPos(window, NULL, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
  }
}

internal void
Win32EndInputRecording(win32_state *state) {
  win32_replay_buffer *buffer = Win32GetReplayBuffer(state, state->inputRecordingIndex);
  CloseHandle(buffer->inputHandle);
  state->inputRecordingIndex = 0;
}

internal void
Win32RecordInput(win32_state *state, game_input *input) {
  win32_replay_buffer *buffer = Win32GetReplayBuffer(state, state->inputRecordingIndex);
  DWORD bytesWritten;
  WriteFile(buffer->inputHandle, input, sizeof(*input), &bytesWritten, 0);
}

internal void
Win32BeginInputPlayingBack(win32_state *state, int index) {
  state->inputPlayingBackIndex = index;
  win32_replay_buffer *buffer = Win32GetReplayBuffer(state, index);

  char filePath[WIN32_MAX_PATH];
  Win32GetReplayInputFilePath(state, index, filePath, sizeof(filePath));

  buffer->inputHandle = CreateFileA(
    filePath,
    GENERIC_READ,
    0,
    0,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    0
  );

  CopyMemory(state->gameMemory, buffer->memory, state->memorySize);
}

internal void
Win32EndInputPlayingBack(win32_state *state) {
  win32_replay_buffer *buffer = Win32GetReplayBuffer(state, state->inputPlayingBackIndex);
  CloseHandle(buffer->inputHandle);
  state->inputPlayingBackIndex = 0;
}

internal void
Win32PlayBackInput(win32_state *state, game_input *input) {
  win32_replay_buffer *buffer = Win32GetReplayBuffer(state, state->inputPlayingBackIndex);
  DWORD bytesRead;
  if(ReadFile(buffer->inputHandle, input, sizeof(*input), &bytesRead, 0)) {
    // hit the end of the stream
    if(bytesRead == 0) {
      int index = state->inputPlayingBackIndex;
      Win32EndInputPlayingBack(state);
      Win32BeginInputPlayingBack(state, index);
      ReadFile(buffer->inputHandle, input, sizeof(*input), &bytesRead, 0);
    }

    Assert(bytesRead == sizeof(*input));
  }
}

inline FILETIME
Win32GetFileLastWriteTime(char *fileName) {
  FILETIME result = {};
  WIN32_FILE_ATTRIBUTE_DATA data;

  if(GetFileAttributesExA(fileName, GetFileExInfoStandard, &data)) {
    result = data.ftLastWriteTime;
  }

  return result;
}

win32_game_code
Win32LoadGameCode(char *dllPath, char *dllTempPath, char *lockPah) {
  win32_game_code result = {};

  WIN32_FILE_ATTRIBUTE_DATA ignored;
  if(!GetFileAttributesExA(lockPah, GetFileExInfoStandard, &ignored)) {
    result.gameDLLLastWriteTime = Win32GetFileLastWriteTime(dllPath);
    // CopyFile may fail the first few times
    while(1) {
      if(CopyFile(dllPath, dllTempPath, false)) break;
      if(GetLastError() == ERROR_FILE_NOT_FOUND) break;
    }

    HMODULE library = LoadLibraryA(dllTempPath);

    if(library) {
      result.gameDLL = library;
      result.gameUpdateVideo = (game_update_video *)(GetProcAddress(library, "GameUpdateVideo"));
      result.gameUpdateAudio = (game_update_audio *)(GetProcAddress(library, "GameUpdateAudio"));
      result.isValid = result.gameUpdateVideo && result.gameUpdateVideo;
    }
  }

  return result;
}

void
Win32UnloadGameCode(win32_game_code *code) {
  if(code->gameDLL) {
    FreeLibrary(code->gameDLL);
    code->gameDLL = 0;
  }

  code->isValid = false;
  code->gameUpdateVideo = NULL;
  code->gameUpdateAudio = NULL;
}

internal real32
Win32ProcessStickValue(int16 value, int16 deadValue) {
  real32 result = 0;

  if(value < -deadValue) {
    result = (real32)(value + deadValue) / (real32)(32768 - deadValue);
  }

  if(value > deadValue) {
    result = (real32)(value - deadValue) / (real32)(32767 - deadValue);
  }

  return result;
}

internal void
Win32DebugDrawVerticalLine(
  win32_offscreen_buffer *buffer,
  int x,
  int top,
  int bottom,
  int width,
  uint32 color
) {
  if(top <= 0) {
    top = 0;
  }
  if(bottom <= 0) {
    bottom = 0;
  }
  if(bottom > buffer->height) {
    bottom = buffer->height;
  }

  if(x >= 0 && x < buffer->width) {
    for(int row = top; row < bottom; row++) {
      uint32 *pixel = (uint32 *)((uint8 *)buffer->memory + row * buffer->width * buffer->bytesPerPixel + x * buffer->bytesPerPixel);

      for(int col = 0; col < width; col++) {
        *pixel++ = color;
      }
    }
  }
}

void
Win32DebugDrawSoundMarker(
  win32_offscreen_buffer *buffer,
  DWORD value,
  real32 c,
  int padX,
  int top,
  int bottom,
  int width,
  uint32 color
) {
  int x = padX + (int)(value * c);
  Win32DebugDrawVerticalLine(buffer, x, top, bottom, width, color);
}

internal void
Win32DebugDrawSoundMarkers(
  win32_offscreen_buffer *buffer,
  win32_debug_sound_marker *markers,
  int markerCount,
  int currentMarkerIndex,
  win32_sound_output *soundOutput
) {
  int padX = 16;
  int padY = 16;
  real32 c = (real32)(buffer->width - 2 * padX) / (real32)soundOutput->bufferSize;

  int lineHeight = 64;

  for(int i = 0; i < markerCount; i++) {
    win32_debug_sound_marker *marker = &markers[i];
    Assert(marker->outputPlayCursor < soundOutput->bufferSize);
    Assert(marker->outputWriteCursor < soundOutput->bufferSize);
    Assert(marker->lockOffset < soundOutput->bufferSize);
    Assert(marker->bytesToLock < soundOutput->bufferSize);
    Assert(marker->flipPlayCursor < soundOutput->bufferSize);
    Assert(marker->flipWriteCursor < soundOutput->bufferSize);

    DWORD playColor = 0xFFFFFFFF;
    DWORD writeColor = 0xFFFF0000;
    DWORD expectedFlipColor = 0xFFFFFF00;
    DWORD playWindowColor = 0xFFFF00FF;

    int top = padY;
    int bottom = top + lineHeight;

    if(i == currentMarkerIndex) {
      top += lineHeight + padY;
      bottom += lineHeight + padY;

      Win32DebugDrawSoundMarker(buffer, marker->outputPlayCursor, c, padX, top, bottom, 1, playColor);
      Win32DebugDrawSoundMarker(buffer, marker->outputWriteCursor, c, padX, top, bottom, 1, writeColor);

      top += lineHeight + padY;
      bottom += lineHeight + padY;

      Win32DebugDrawSoundMarker(buffer, marker->lockOffset, c, padX, top, bottom, 1, playColor);
      Win32DebugDrawSoundMarker(buffer, marker->bytesToLock + marker->lockOffset, c, padX, top, bottom, 1, writeColor);

      top += lineHeight + padY;
      bottom += lineHeight + padY;

      Win32DebugDrawSoundMarker(buffer, marker->expectedFlipPlayCursor, c, padX, top, bottom, 1, expectedFlipColor);
    }

    Win32DebugDrawSoundMarker(buffer, marker->flipPlayCursor, c, padX, top, bottom, 1, playColor);
    Win32DebugDrawSoundMarker(buffer, marker->flipPlayCursor + 480 * soundOutput->bytesPerSample, c, padX, top, bottom, 1, playWindowColor);
    Win32DebugDrawSoundMarker(buffer, marker->flipWriteCursor, c, padX, top, bottom, 1, writeColor);
  }
}

internal void
Win32ProcessKeyboardButtonState(
  game_button_state *newButtonState,
  bool32 isDown
) {
  if(newButtonState->isEndedDown != isDown) {
    newButtonState->isEndedDown = isDown;
    newButtonState->halfTransitionCount++;
  }
}

internal void
Win32ProcessXInputButtonState(
  game_button_state *newButtonState,
  game_button_state *OldButtonState,
  DWORD buttons,
  DWORD buttonBit
) {
  newButtonState->halfTransitionCount = OldButtonState->isEndedDown == newButtonState->isEndedDown ? 0 : 1;
  newButtonState->isEndedDown = buttons & buttonBit;
}

internal void
Win32ProcessMessages(win32_state *win32State, game_controller_input *keyboardController) {
  MSG message = {};

  while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
    switch(message.message) {
      case WM_SYSKEYUP:
      case WM_SYSKEYDOWN:
      case WM_KEYUP:
      case WM_KEYDOWN: {
        uint32 VKCode = (uint32)message.wParam;
        bool wasDown = (message.lParam & (1 << 30)) != 0;
        bool isDown = (message.lParam & (1 << 31)) == 0;

        if(isDown) {
          bool32 isAltDown = message.lParam & (1 << 29);

          if(VKCode == VK_F4 && isAltDown) {
            globalRunning = false;
          }

          if(VKCode == VK_RETURN && isAltDown) {
            ToggleFullscreen(message.hwnd);
          }
        }

        if(wasDown != isDown) {
          switch(VKCode) {
            case 'W': {
              Win32ProcessKeyboardButtonState(&keyboardController->moveUp, isDown);
            } break;
            case 'A': {
              Win32ProcessKeyboardButtonState(&keyboardController->moveLeft, isDown);
            } break;
            case 'S': {
              Win32ProcessKeyboardButtonState(&keyboardController->moveDown, isDown);
            } break;
            case 'D': {
              Win32ProcessKeyboardButtonState(&keyboardController->moveRight, isDown);
            } break;

            case 'p': {
              if(message.message == WM_KEYDOWN) {
                globalDebugUpdateWindow = !globalDebugUpdateWindow;
              }
            };

            case VK_UP: {
              Win32ProcessKeyboardButtonState(&keyboardController->actionUp, isDown);
            } break;

            case VK_DOWN: {
              Win32ProcessKeyboardButtonState(&keyboardController->actionDown, isDown);
            } break;

            case VK_LEFT: {
              Win32ProcessKeyboardButtonState(&keyboardController->actionLeft, isDown);
            } break;

            case VK_RIGHT: {
              Win32ProcessKeyboardButtonState(&keyboardController->actionRight, isDown);
            } break;

            case VK_ESCAPE: {
              Win32ProcessKeyboardButtonState(&keyboardController->back, isDown);
            } break;

            case VK_SPACE: {
              Win32ProcessKeyboardButtonState(&keyboardController->start, isDown);
            } break;

            // left shoulder
            case 'Q': {
              Win32ProcessKeyboardButtonState(&keyboardController->leftShoulder, isDown);
            } break;

            // right shoulder
            case 'E': {
              Win32ProcessKeyboardButtonState(&keyboardController->rightShoulder, isDown);
            } break;

            case 'L': {
              if(isDown) {
                if(win32State->inputRecordingIndex == 0) {
                  if(win32State->inputPlayingBackIndex == 0) {
                    Win32BeginInputRecording(win32State, 1);
                  } else {
                    Win32EndInputPlayingBack(win32State);
                    // Reset all buttons
                    for(int i = 0; i < ArrayCount(keyboardController->buttons); i++) {
                      keyboardController->buttons[i] = {};
                    }
                  }
                } else {
                  Win32EndInputRecording(win32State);
                  Win32BeginInputPlayingBack(win32State, 1);
                }
              }
            } break;
          }
        }
      } break;

      default: {
        TranslateMessage(&message);
        DispatchMessage(&message);
      } break;
    }
  }
}

internal void
Win32InitDSound(HWND window, int32 samplesPerSecond, int32 bufferSize) {
  HMODULE library = LoadLibraryA("dsound.dll");
  if(library) {
    direct_sound_create *directSoundCreate = (direct_sound_create *)GetProcAddress(library, "DirectSoundCreate");

    LPDIRECTSOUND directSound;

    if(SUCCEEDED(directSoundCreate(0, &directSound, 0))) {
      if(SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY))) {
        // OutputDebugStringA("Set cooperative level ok\n");
      } else {
        // TODO: logging
      }

      WAVEFORMATEX waveFormat  = {};
      waveFormat.wFormatTag = WAVE_FORMAT_PCM;
      waveFormat.nChannels = 2;
      waveFormat.nSamplesPerSec = samplesPerSecond;
      waveFormat.wBitsPerSample = 16;
      waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
      waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

      {
        DSBUFFERDESC bufferDesc = {};
        bufferDesc.dwSize = sizeof(bufferDesc);
        bufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
        LPDIRECTSOUNDBUFFER primaryBuffer;
        if(SUCCEEDED(directSound->CreateSoundBuffer(&bufferDesc, &primaryBuffer, 0))) {
          // OutputDebugStringA("Create primary buffer ok\n");
          if(SUCCEEDED(primaryBuffer->SetFormat(&waveFormat))) {
            // OutputDebugStringA("Primary buffer set format ok\n");
          } else {
            // TDOO: logging
          }
        }
      }

      {
        DSBUFFERDESC bufferDesc = {};
        bufferDesc.dwSize = sizeof(bufferDesc);
        bufferDesc.dwFlags = 0;
        bufferDesc.dwBufferBytes = bufferSize;
        bufferDesc.lpwfxFormat = &waveFormat;
        if(SUCCEEDED(directSound->CreateSoundBuffer(&bufferDesc, &globalSoundBuffer, 0))) {
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
Win32ClearSoundBuffer(win32_sound_output *soundOutput) {
  void *region1;
  DWORD region1Size;
  void *region2;
  DWORD region2Size;

  if(SUCCEEDED(globalSoundBuffer->Lock(
    0,
    soundOutput->bufferSize,
    &region1, &region1Size,
    &region2, &region2Size,
    0
  ))) {
    uint8 *Out = (uint8 *)region1;

    for(DWORD byteIndex = 0; byteIndex < region1Size; byteIndex++) {
      *Out++ = 0;
    }

    Out = (uint8 *)region2;

    for(DWORD byteIndex = 0; byteIndex < region2Size; byteIndex++) {
      *Out++ = 0;
    }

    globalSoundBuffer->Unlock(region1, region1Size, region2, region2Size);
  }
}

internal void
Win32FillSoundBuffer(
  win32_sound_output *soundOutput,
  game_sound_buffer *sourceBuffer,
  DWORD lockOffset, DWORD bytesToLock
) {
  void *region1;
  DWORD region1Size;
  void *region2;
  DWORD region2Size;

  if(SUCCEEDED(globalSoundBuffer->Lock(
    lockOffset,
    bytesToLock,
    &region1, &region1Size,
    &region2, &region2Size,
    0
  ))) {
    int16 *dest = (int16 *)region1;
    int16 *source = sourceBuffer->memory;

    for(DWORD sampleIndex = 0; sampleIndex < region1Size / soundOutput->bytesPerSample; sampleIndex++) {
      *dest++ = *source++;
      *dest++ = *source++;
      soundOutput->runningSampleIndex++;
    }

    dest = (int16 *)region2;
    for(DWORD sampleIndex = 0; sampleIndex < region2Size / soundOutput->bytesPerSample; sampleIndex++) {
      *dest++ = *source++;
      *dest++ = *source++;
      soundOutput->runningSampleIndex++;
    }

    globalSoundBuffer->Unlock(region1, region1Size, region2, region2Size);
  }
}

internal void
Win32LoadXInput() {
  HMODULE library = LoadLibraryA("xinput1_4.dll");
  if(library) {
    XInputGetState = (x_input_get_state *)GetProcAddress(library, "XInputGetState");
    XInputSetState = (x_input_set_state *)GetProcAddress(library, "XInputSetState");
  }
}

internal win32_window_dimension
Win32GetWindowDimension(HWND window) {
  win32_window_dimension result;
  RECT clientRect;
  GetClientRect(window, &clientRect);
  result.width = clientRect.right - clientRect.left;
  result.height = clientRect.bottom - clientRect.top;
  return result;
}

internal void
Win32ResizeDIBSection(
  win32_offscreen_buffer *buffer,
  int width,
  int height
) {
  if(buffer->memory) {
    VirtualFree(buffer->memory, 0, MEM_RELEASE);
  }

  buffer-> width = width;
  buffer-> height = height;
  buffer->bytesPerPixel = 4;

  buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
  buffer->info.bmiHeader.biWidth = buffer->width;
  buffer->info.bmiHeader.biHeight = buffer->height;
  buffer->info.bmiHeader.biPlanes = 1;
  buffer->info.bmiHeader.biBitCount = (WORD)(buffer->bytesPerPixel * 8);
  buffer->info.bmiHeader.biCompression = BI_RGB;

  int BitmapSize = buffer->width * buffer->height * buffer->bytesPerPixel;

  buffer->memory = VirtualAlloc(0, BitmapSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
}

internal void
Win32UpdateWindow(
  HDC deviceContext,
  int windowWidth,
  int windowHeight,
  win32_offscreen_buffer *buffer
) {
  if(windowWidth >= 2*buffer->width && windowHeight >= 2*buffer->height) {
    StretchDIBits(
      deviceContext,
      0, 0, 2*buffer->width, 2*buffer->height, // dest
      0, 0, buffer->width, buffer->height, // src
      buffer->memory,
      &buffer->info,
      DIB_RGB_COLORS,
      SRCCOPY
    );
  } else {
    int top = 10;
    int left = 10;

    // four gutters
    PatBlt(deviceContext, 0, 0, windowWidth, top, BLACKNESS);
    PatBlt(deviceContext, 0, top, left, buffer->height, BLACKNESS);
    PatBlt(deviceContext, 0, top + buffer->height, windowWidth, windowHeight - buffer->height - top, BLACKNESS);
    PatBlt(deviceContext, left + buffer->width, top, windowWidth - left - buffer->width, buffer->height, BLACKNESS);

    StretchDIBits(
      deviceContext,
      top, left, buffer->width, buffer->height, // dest
      0, 0, buffer->width, buffer->height, // src
      buffer->memory,
      &buffer->info,
      DIB_RGB_COLORS,
      SRCCOPY
    );
  }
}

internal LRESULT CALLBACK
Win32MainWindowCallback(
  HWND window,
  UINT message,
  WPARAM wParam,
  LPARAM lParam
) {
  LRESULT result = 0;
  switch(message) {
    case WM_SETCURSOR: {
      if(globalShowCursor) {
        result = DefWindowProcA(window, message, wParam, lParam);
      } else {
        SetCursor(0);
      }
    } break;

    case WM_ACTIVATEAPP: {
      OutputDebugStringA("WM_ACTIVATEAPP\n");
    } break;

    case WM_CLOSE: {
      globalRunning = false;
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
      PAINTSTRUCT paintStruct = {};
      HDC deviceContext = BeginPaint(window, &paintStruct);
      win32_window_dimension dimension = Win32GetWindowDimension(window);
      Win32UpdateWindow(
        deviceContext,
        dimension.width,
        dimension.height,
        &globalBackBuffer
      );
      EndPaint(window, &paintStruct);
    } break;

    case WM_SIZE:
    {
      OutputDebugStringA("WM_SIZE\n");
    } break;

    default:
    {
      result = DefWindowProcA(window, message, wParam, lParam);
    } break;
  }

  return result;
}

inline real32
Win32GetSecondsElapsed(uint64 start, uint64 End) {
  real32 result = 0;
  result = (real32)(End - start) / (real32)globalPerfCounterFrequency;
  return result;
}

inline uint64
Win32GetPerfCounter() {
  LARGE_INTEGER result;
  QueryPerformanceCounter(&result);
  return result.QuadPart;
}

#define SoundFrameLatency 3

int CALLBACK
WinMain(
  HINSTANCE instance,
  HINSTANCE prevInstance,
  LPSTR cmdLine,
  int showCmd
) {
#if HANDMADE_INTERNAL
  globalShowCursor = true;
#endif
  bool32 sleepIsGranular = timeBeginPeriod(1) == TIMERR_NOERROR;

  LARGE_INTEGER counterPerSecond;
  QueryPerformanceFrequency(&counterPerSecond);
  globalPerfCounterFrequency = counterPerSecond.QuadPart;

  Win32LoadXInput();

  WNDCLASS WindowClass = {};

  WindowClass.style = CS_HREDRAW | CS_VREDRAW;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = instance;
  WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";

  if(RegisterClass(&WindowClass)) {
    thread_context thread = {};
    HWND window = CreateWindowEx(
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
      instance,
      0
    );

    if(window) {
      HDC deviceContext = GetDC(window);
      int monitorRefreshHz = GetDeviceCaps(deviceContext, VREFRESH);
      int gameUpdateHz = monitorRefreshHz / 2;
      real32 targetSecondsPerFrame = 1.0f / (real32)gameUpdateHz;

      win32_sound_output soundOutput = {};
      soundOutput.samplesPerSecond = 48000;
      soundOutput.bytesPerSample = 4;
      soundOutput.safetyBytes = (soundOutput.samplesPerSecond * soundOutput.bytesPerSample) / gameUpdateHz / 3;
      soundOutput.bufferSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
      soundOutput.toneVolume = 3000;
      soundOutput.runningSampleIndex = 0;
      int16 *soundMemory = (int16 *)VirtualAlloc(0, soundOutput.bufferSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);

      game_memory memory = {};
      memory.debugPlatformReadFile = DebugPlatformReadFile;
      memory.debugPlatformWriteFile = DebugPlatformWriteFile;
      memory.debugPlatformFreeFileMemory = DebugPlatformFreeFileMemory;
      memory.permanentStorageSize = Megabytes(64);
      memory.transientStorageSize = Megabytes(1);
#ifdef HANDMADE_INTERNAL
      LPVOID baseAddress = (LPVOID)Terabytes(2);
#else
      LPVOID baseAddress = 0;
#endif
      memory.permanentStorage = VirtualAlloc(
        baseAddress,
        (size_t)(memory.permanentStorageSize + memory.transientStorageSize),
        MEM_COMMIT|MEM_RESERVE,
        PAGE_READWRITE
      );
      memory.transientStorage = (uint8 *)memory.permanentStorage + memory.permanentStorageSize;

      win32_state win32State = {};
      win32State.gameMemory = memory.permanentStorage;
      win32State.memorySize = memory.permanentStorageSize + memory.transientStorageSize;

      for(int i = 0; i < ArrayCount(win32State.replayBuffers); i++) {
        win32State.replayBuffers[i].memory = VirtualAlloc(
          0,
          win32State.memorySize,
          MEM_COMMIT|MEM_RESERVE,
          PAGE_READWRITE
        );
        Assert(win32State.replayBuffers[i].memory != 0);
      }

      if(memory.permanentStorage && soundMemory) {
        Win32InitDSound(window, soundOutput.samplesPerSecond, soundOutput.bufferSize);
        Win32ClearSoundBuffer(&soundOutput);
        globalSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);

        Win32ResizeDIBSection(&globalBackBuffer, 960, 540);

        game_input input[2] = {};
        game_input *oldInput = &input[0];
        game_input *newInput = &input[1];

        uint64 lastCounter = Win32GetPerfCounter();
        uint64 flipCounter = Win32GetPerfCounter();
        uint64 lastCycleCounter = __rdtsc();

        bool32 soundIsValid = false;
#if 0
        int DebugSoundMarkerIndex = 0;
        win32_debug_sound_marker DebugSoundMarkers[15] = {};
#endif
        Win32GetExePath(&win32State);

        char gameDLLName[] = "handmade.dll";
        char gameDLLPath[WIN32_MAX_PATH];
        Win32BuildPathInExeDir(
          &win32State,
          gameDLLName,
          sizeof(gameDLLName) - 1,
          gameDLLPath,
          sizeof(gameDLLPath)
        );

        char gameTempDLLName[] = "handmade_temp.dll";
        char gameTempDLLPath[WIN32_MAX_PATH];
        Win32BuildPathInExeDir(
          &win32State,
          gameTempDLLName,
          sizeof(gameTempDLLName) - 1,
          gameTempDLLPath,
          sizeof(gameTempDLLPath)
        );

        char gameLockFileName[] = "lock.tmp";
        char gameLockFilePath[WIN32_MAX_PATH];
        Win32BuildPathInExeDir(
          &win32State,
          gameLockFileName,
          sizeof(gameLockFileName) - 1,
          gameLockFilePath,
          sizeof(gameLockFilePath)
        );

        win32_game_code game = Win32LoadGameCode(gameDLLPath, gameTempDLLPath, gameLockFilePath);

        while(globalRunning) {
#if 0
          // write cursor - last write cursor: 1920 bytes = 480 samples
          DWORD playCursor, writeCursor;
          globalSoundBuffer->GetCurrentPosition(&playCursor, &writeCursor);
          char buffer[256];
          sprintf_s(buffer, sizeof(buffer), "PC: %d, WC: %d\n", playCursor, writeCursor);
          OutputDebugStringA(buffer);
#endif
          FILETIME newDLLWriteTime = Win32GetFileLastWriteTime(gameDLLPath);
          if(CompareFileTime(&newDLLWriteTime, &game.gameDLLLastWriteTime) != 0) {
            Win32UnloadGameCode(&game);
            game = Win32LoadGameCode(gameDLLPath, gameTempDLLPath, gameLockFilePath);
          }

          newInput->dt = targetSecondsPerFrame;

          // keyboard is always the first controller
          game_controller_input *newKeyboardController = &newInput->controllers[0];
          game_controller_input *oldKeyboardController = &oldInput->controllers[0];
          *newKeyboardController = {};
          newKeyboardController->isConnected = true;

          for(int i = 0; i < ArrayCount(newKeyboardController->buttons); i++) {
            newKeyboardController->buttons[i].isEndedDown = oldKeyboardController->buttons[i].isEndedDown;
          }

          Win32ProcessMessages(&win32State, newKeyboardController);

          int controllerCount = XUSER_MAX_COUNT;
          if(controllerCount > (ArrayCount(newInput->controllers) - 1)) {
            controllerCount = ArrayCount(newInput->controllers) - 1;
          }

          for(int i=0; i< controllerCount; i++) {
            XINPUT_STATE state;

            game_controller_input *oldController = &oldInput->controllers[i + 1];
            game_controller_input *newController = &newInput->controllers[i + 1];

            ZeroMemory(&state, sizeof(XINPUT_STATE));

            if(XInputGetState(i, &state) == ERROR_SUCCESS ) {
              newController->isConnected = true;
              newController->isAnalog = oldController->isAnalog;
              XINPUT_GAMEPAD *pad = &state.Gamepad;

              newController->stickAverageX = Win32ProcessStickValue(pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
              newController->stickAverageY = Win32ProcessStickValue(pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

              if(newController->stickAverageX != 0.0f || newController->stickAverageY != 0.0f) {
                newController->isAnalog = true;
              }

              if(pad->wButtons &XINPUT_GAMEPAD_DPAD_UP) {
                newController->stickAverageY = 1.0f;
                newController->isAnalog = false;
              }
              if(pad->wButtons &XINPUT_GAMEPAD_DPAD_DOWN) {
                newController->stickAverageY = -1.0f;
                newController->isAnalog = false;
              }
              if(pad->wButtons &XINPUT_GAMEPAD_DPAD_RIGHT) {
                newController->stickAverageY = 1.0f;
                newController->isAnalog = false;
              }
              if(pad->wButtons &XINPUT_GAMEPAD_DPAD_LEFT) {
                newController->stickAverageY = -1.0f;
                newController->isAnalog = false;
              }

              real32 threshold = 0.5f;
              Win32ProcessXInputButtonState(
                &newController->moveUp, &oldController->moveUp,
                (newController->stickAverageY > threshold ? 1 : 0), 1
              );
              Win32ProcessXInputButtonState(
                &newController->moveDown, &oldController->moveDown,
                (newController->stickAverageY < -threshold ? 1 : 0), 1
              );
              Win32ProcessXInputButtonState(
                &newController->moveRight, &oldController->moveRight,
                (newController->stickAverageX > threshold ? 1 : 0), 1
              );
              Win32ProcessXInputButtonState(
                &newController->moveLeft, &oldController->moveLeft,
                (newController->stickAverageX < -threshold ? 1 : 0), 1
              );

              Win32ProcessXInputButtonState(
                &newController->actionDown, &oldController->actionDown,
                pad->wButtons, XINPUT_GAMEPAD_A
              );
              Win32ProcessXInputButtonState(
                &newController->actionRight, &oldController->actionRight,
                pad->wButtons, XINPUT_GAMEPAD_B
              );
              Win32ProcessXInputButtonState(
                &newController->actionLeft, &oldController->actionLeft,
                pad->wButtons, XINPUT_GAMEPAD_X
              );
              Win32ProcessXInputButtonState(
                &newController->actionUp, &oldController->actionUp,
                pad->wButtons, XINPUT_GAMEPAD_Y
              );
              Win32ProcessXInputButtonState(
                &newController->leftShoulder, &oldController->leftShoulder,
                pad->wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER
              );
              Win32ProcessXInputButtonState(
                &newController->rightShoulder, &oldController->rightShoulder,
                pad->wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER
              );
              Win32ProcessXInputButtonState(
                &newController->start, &oldController->start,
                pad->wButtons, XINPUT_GAMEPAD_START
              );
              Win32ProcessXInputButtonState(
                &newController->back, &oldController->back,
                pad->wButtons, XINPUT_GAMEPAD_BACK
              );
            }
            else{
              // Controller is not connected
            }
          }

          // Update video
          {
            game_offscreen_buffer buffer = {};
            buffer.memory = globalBackBuffer.memory;
            buffer.bytesPerPixel = globalBackBuffer.bytesPerPixel;
            buffer.width = globalBackBuffer.width;
            buffer.height = globalBackBuffer.height;
            buffer.pitch = buffer.width * buffer.bytesPerPixel;

            if(win32State.inputRecordingIndex != 0) {
              Win32RecordInput(&win32State, newInput);
            }

            if(win32State.inputPlayingBackIndex != 0) {
              Win32PlayBackInput(&win32State, newInput);
            }

            // Collect mouse info
            {
              POINT point;
              GetCursorPos(&point);
              ScreenToClient(window, &point);
              newInput->mouseX = int32(point.x);
              newInput->mouseY = int32(point.y);
              newInput->mouseButtons[0].isEndedDown = GetKeyState(VK_LBUTTON) & (1 << 15);
              newInput->mouseButtons[1].isEndedDown = GetKeyState(VK_RBUTTON) & (1 << 15);
              newInput->mouseButtons[2].isEndedDown = GetKeyState(VK_MBUTTON) & (1 << 15);
              newInput->mouseButtons[3].isEndedDown = GetKeyState(VK_XBUTTON1) & (1 << 15);
              newInput->mouseButtons[4].isEndedDown = GetKeyState(VK_XBUTTON2) & (1 << 15);
            }

            if(game.gameUpdateVideo) {
              game.gameUpdateVideo(&thread, &memory, newInput, &buffer);
            }
          }

          // Update Audio
          {
            uint64 audioCounter = Win32GetPerfCounter();
            real32 fromBeginToAudioSeconds = Win32GetSecondsElapsed(flipCounter, audioCounter);

            DWORD playCursor, writeCursor;
            if(globalSoundBuffer->GetCurrentPosition(&playCursor, &writeCursor) == DS_OK) {
              if(!soundIsValid) {
                soundOutput.runningSampleIndex = writeCursor / soundOutput.bytesPerSample;
                soundIsValid = true;
              }

              DWORD lockOffset = soundOutput.runningSampleIndex * soundOutput.bytesPerSample % soundOutput.bufferSize;
              DWORD soundBytesPerFrame = (soundOutput.samplesPerSecond * soundOutput.bytesPerSample) / gameUpdateHz;
              real32 secondsLeftUntilFlip = targetSecondsPerFrame - fromBeginToAudioSeconds;
              DWORD expectedBytesUntilFlip = (DWORD)(secondsLeftUntilFlip / targetSecondsPerFrame * (real32)soundBytesPerFrame);
              DWORD expectedFrameBoundaryByte = playCursor + expectedBytesUntilFlip;
              DWORD safeWriteCursor = writeCursor;
              if(safeWriteCursor < playCursor) {
                safeWriteCursor += soundOutput.bufferSize;
              }
              Assert(safeWriteCursor >= playCursor);
              safeWriteCursor += soundOutput.safetyBytes;
              bool32 audioIsLowLatency = safeWriteCursor <= expectedFrameBoundaryByte;
              DWORD targetCursor = 0;
              if(audioIsLowLatency) {
                targetCursor = expectedFrameBoundaryByte + soundBytesPerFrame;
              } else {
                targetCursor = writeCursor + soundBytesPerFrame + soundOutput.safetyBytes;
              }
              targetCursor = targetCursor % soundOutput.bufferSize;

              DWORD bytesToLock = 0;
              if(lockOffset > targetCursor) {
                bytesToLock = soundOutput.bufferSize - lockOffset + targetCursor;
              } else {
                bytesToLock = targetCursor - lockOffset;
              }

              game_sound_buffer soundBuffer = {};
              soundBuffer.samplesPerSecond = soundOutput.samplesPerSecond;
              soundBuffer.sampleCount = bytesToLock / soundOutput.bytesPerSample;
              soundBuffer.toneVolume = soundOutput.toneVolume;
              soundBuffer.memory = soundMemory;
              if(game.gameUpdateAudio) {
                game.gameUpdateAudio(&thread, &memory, &soundBuffer);
              }
#if 0
              win32_debug_sound_marker *marker = &DebugSoundMarkers[DebugSoundMarkerIndex];
              marker->outputPlayCursor = playCursor;
              marker->outputWriteCursor = writeCursor;
              marker->lockOffset = lockOffset;
              marker->bytesToLock = bytesToLock;
              marker->expectedFlipPlayCursor = expectedFrameBoundaryByte;

              DWORD UnwrappedWriteCursor = writeCursor;
              if(UnwrappedWriteCursor < playCursor) {
                UnwrappedWriteCursor += soundOutput.bufferSize;
              }
              DWORD AudioLantencyBytes = UnwrappedWriteCursor - playCursor;
              real32 AudioLatencySeconds = (real32)(AudioLantencyBytes / soundOutput.bytesPerSample) / (real32)soundOutput.samplesPerSecond;

              char AudioTextBuffer[256];
              sprintf_s(AudioTextBuffer, "lockOffset: %d, targetCursor: %d, bytesToLock: %d, playCursor: %d, writeCursor: %d, AudioLatencyBytes: %d, AudioLatencySeconds: %f\n", lockOffset, targetCursor, bytesToLock, playCursor, writeCursor, AudioLantencyBytes, AudioLatencySeconds);
              // OutputDebugStringA(AudioTextBuffer);
#endif
              Win32FillSoundBuffer(&soundOutput, &soundBuffer, lockOffset, bytesToLock);
            } else {
              soundIsValid = false;
            }
          }

          // Lock the frame rate
          real32 secondsElapsed = Win32GetSecondsElapsed(lastCounter, Win32GetPerfCounter());
          if(secondsElapsed < targetSecondsPerFrame) {
            if(sleepIsGranular) {
              DWORD sleepMS = (DWORD)(1000 * (targetSecondsPerFrame - secondsElapsed));
              if(sleepMS > 0) {
                Sleep(sleepMS);
              }
            }

            while(secondsElapsed < targetSecondsPerFrame) {
              secondsElapsed = Win32GetSecondsElapsed(lastCounter, Win32GetPerfCounter());
            }
          } else {
            // We missed a frame
            // TODO: logging
          }

          uint64 endCounter = Win32GetPerfCounter();
#if 0
          Win32DebugDrawSoundMarkers(
            &globalBackBuffer,
            DebugSoundMarkers,
            ArrayCount(DebugSoundMarkers),
            DebugSoundMarkerIndex - 1, // maybe -1, we don't care
            &soundOutput
          );
#endif
          if(globalDebugUpdateWindow) {
            win32_window_dimension dimension = Win32GetWindowDimension(window);
            HDC dC = GetDC(window);
            Win32UpdateWindow(
              dC,
              dimension.width,
              dimension.height,
              &globalBackBuffer
            );
          }

          flipCounter = Win32GetPerfCounter();
#if 0
          {
            DWORD playCursor, writeCursor;
            if(SUCCEEDED(globalSoundBuffer->GetCurrentPosition(&playCursor, &writeCursor))) {
            win32_debug_sound_marker *marker = &DebugSoundMarkers[DebugSoundMarkerIndex];
            marker->flipPlayCursor = playCursor;
            marker->flipWriteCursor = writeCursor;
            }
          }
          DebugSoundMarkerIndex++;
          if(DebugSoundMarkerIndex == ArrayCount(DebugSoundMarkers)) {
            DebugSoundMarkerIndex = 0;
          }
#endif
          game_input *tmp = oldInput;
          oldInput = newInput;
          newInput = tmp;

          // Performance counter
          uint64 currentCycleCounter = __rdtsc();
#if 0
          int64 CounterElapsed = endCounter - lastCounter;
          uint64 CycleElapsed = currentCycleCounter - lastCycleCounter;

          real32 MSPerFrame = 1000.0f * (real32)CounterElapsed / (real32)counterPerSecond.QuadPart;
          real32 FPS = (real32)counterPerSecond.QuadPart / (real32)CounterElapsed;
          real32 MCPF = (real32)CycleElapsed / (1000.0f * 1000.0f);

          char OutputBuffer[256];
          sprintf_s(OutputBuffer, sizeof(OutputBuffer), "ms/f: %.2f,  fps: %.2f,  mc/f: %.2f\n", MSPerFrame, FPS, MCPF);
          OutputDebugStringA(OutputBuffer);
#endif
          lastCounter = endCounter;
          lastCycleCounter = currentCycleCounter;
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
