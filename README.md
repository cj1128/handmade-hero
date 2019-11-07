<div align="center">
  <h1>
    <a href="https://handmadehero.org/">Handmade Hero</a>
  </h1>

  <img src="./home.png">
</div>

## Intro

This is my personal study notes of [Handmade Hero](https://handmadehero.org).

If you think **writing a professional-quality game from scratch on your own (no engine no library)** is interesting and challenging, I highly recommend this project. In my opinion, it's the best I can find.

Deepest thanks to *Casey Muratori* for this excellent project. It's really helpful.

## Env

Windows 10 with Visual Studio 2019 and Sublime Text 3.

## Windows Programming

### Bash

- `dir /s [keyword]`: search something

### Win32

- `WS_EX_TOPMOST`: make window in front of others
- `WS_EX_LAYERED` and `SetLayeredWindowAttributes `: change window alpha

## Roadmap

### Day 1: Setting Up the Windows Build

- Install Visual Studio 2019
- Call `vsdevcmd` to init command line tools
- Use `cl` to build our program
- Use `devenv` to start visual studio to debug, e.g. `devenv w:\build\win32_handmade.exe`
- `WinMain`: windows program entry
- `MessageBox`: show a message box

### Day 2: Opening a Win32 Window

- `WNDCLASS`, `RegisterClass`
- `GetModuleHandle`
- `OutputDebugString`
- `DefWindowProc`
- `CreateWindow`, `CreateWindowEx`
- `GetMessage`, `TranslateMessage`, `DispatchMessage`
- `BeginPaint`, `EndPaint`, `PatBlt`

### Day 3: Allocating a Back Buffer

- `PostQuitMessage`
- #define `global_variable` and `internal` to `static`
- Resize buffer when receive WM_RESIZE
- `GetClientRect`
- `CreateDIBSection`
- `StretchDIBits`
- `DeleteObject`
- `CreateCompatibleDC`
- `ReleaseDC`

### Day 4: Animating the Back Buffer

- Use `VirtualAlloc` to alloc bit map memory instead of `CreateDIBSection`
- `VirtualFree`, `VirtualProtect`
- Set `biHeight` to negative value so we the image origin if top-left
- Render a simple gradient. Each pixel has a value of form `0xXXRRGGBB`
- use `PeekMessage` instead of `GetMessage`, because it doesn't block
- `GetDC`, `ReleaseDC`

### Day 5: Windows Graphics Review

- `HREDRAW` and `VREDRAW` are used to tell Windows to redraw the whole window
- Use `win32_offscreen_buffer` to bundle all global variables
- Create the back buffer just once, move it out of `WM_SIZE`

### Day 6: Gamepad and Keyboard Input

- `XInput`, `XInputGetState`, `XInputSetState`, `XUSER_MAX_COUNT`
- Loading windows functions ourselves
- Use XInput 1.3
- `LoadLibrary`, `GetProcAddress`
- `WM_SYSKEYUP`, `WM_SYSKEYDOWN`, `WM_KEYUP`, `WM_KEYDOWN`
- Get IsDown and WasDown status from LParam

### Day 7: Initializing DirectSound

- Return `ERROR_DEVICE_NOT_CONNECTED` in xinput stub functions
- Implement `Alt+F4` to close the window
- Use bool32 if we only care if the value is 0 or not 0
- `dsound.h`, [IDirectSound8 Interface](https://docs.microsoft.com/en-us/previous-versions/windows/desktop/ee418035(v=vs.85))
- `DirectSoundCreate`, `SetCooperativeLevel`, `CreateSoundBuffer`, `SetFormat`
- Remember to clear `DSBUFFERDESC` to zero
- Add `MEM_RESERVE` to `VirtualAlloc`

### Day 8: Writing a Square Wave to DirectSound

- [IDirectSouondBuffer8 Interface](https://docs.microsoft.com/en-us/previous-versions/windows/desktop/ee418055%28v%3dvs.85%29)
- `Lock`, `Unlock`, `GetCurrentPosition`, `Play`

### Day 9: Variable-Pitch Sine Wave Output

- `sinf`
- `win32_sound_output`, `Win32FillSoundBuffer`
- `tSine`, `LatencySampleCount`
- We need to handle xinput deadzone in the future
- Use `DefWindowProcA` instead of `DefWindowProc`

### Day 10: QueryPerformanceCounter and RDTSC

- `QueryPerformanceCounter`, `LARGE_INTEGER`, `QuyerPerformanceFrequency`
- `wsprintf`, `__rdtsc`
- Intrinsic: looks like a function call, but it's used to tell the compiler we want a specific assembly instruction here

### Day 11: The Basics of Platform API Design

- Win32 platform todo list:
  - Saved game location
  - Getting a handle to our executable
  - Asset loading
  - Threading
  - Raw input (support for multiple keyboards)
  - Sleep/timeBeginPeriod
  - ClipCursor() (for multimonitor)
  - Fullscreen
  - WM_SETCURSOR (control cursor visibility)
  - QueryCancelAutoplay
  - WM_ACTIVATEAPP (for when we are not the active application)
  - Blit speed improvements
  - Hardware acceleration
  - GetKeyboardLayout (for French keyboards)
- For each platform, we will have a big [platform]\_handmade.cpp file. Inside this file, we #include other files.
- Treat our game as a service, rather than the operating system.

### Day 12: Platform-Independent Sound Output

- `_alloca`: Allocate some memory in the stack, freeed when the function exists rather than leave out of the enclosing scope
- Move sound rendering logic to handmade.cpp

### Day 13: Platform-Independent User Input

- Define `game_input`, `game_controller_input`, `game_button_state`
- Store OldInput and NewInput and do ping-pang at end of every frame
- Define `ArrayCount` macro

### Day 14: Platform-Independent Game Memory

- Use a `game_memory` struct to handle all memory related stuff
- We have permannent storage and trasient storage in our memory
- Define `Kilobytes`, `Megabytes` and `GigaBytes` macros
- We require the memory allocated to be cleared to zero
- Define `Assert` macro
- Use `cl -Dname=val` to define `HANDMADE_INTERNAL` and `HANDMADE_SLOW` compiler flags
- Specify base address when we do `VirtualAlloc` for debugging purpose in internal build

### Day 15: Platform-Independent Debug File

- Define `DebugPlatformReadFile`, `DebugPlatformWriteFile` and `DebugPlatformFreeFileMemory` only when we are using internal build
- Define `SafeTruncateUInt64` inline functions
- `CreateFile`, `GetFileSizeEx`, `ReadFile`
- `__FIEL__` is a compile time macro points to current file

### Day 16: Visual Studio Compiler Switches
- VS compiler switches:
  - `-WX`, `-W4`: enable warning level 4 and treat warnings as errors
  - `-wd`: turn off some warnings
  - `-MT`: static link C runtime library
  - `-Oi`: generates intrinsic functions.
  - `-Od`: disable optimization
  - `-GR-`: disable run-time type information, we don't need this
  - `-Gm-`: disable minimal rebuild
  - `-EHa-`: disable exception-handling
  - `-nologo`: don't print compiler info
  - `-FC`: full Path of Source Code File in Diagnostics
- Init `vsdevcmd` using `-arch=x86` flags to build a 32-bit version of our program
- Use `/link` to pass linker options to make a valid Windows XP executable
  - `-subsystem:windows,5.1`

### Day 17: Unified Keyboard and Gamepad Input

- Add one controller, so we have 5 controllers now
- Extract `CommonCompilerFlags` and `CommonLinkerFlags` in build.bat
- Copy old keyboard button state to new keyboard button state
- Add MoveUp, MoveDown, MoveLeft, MoveRight buttons
- Handle XInput dead zone
- Check whether union in game_controller_input is aligned

### Day 18: Enforcing a Video Frame Rate

- We need to find a way to reliably retrieve monitor refresh rate?
- We define `GameUpdateHz` based on `MonitorRefreshHz`
- Use `Sleep` to wait for the remaining time
- Use `timeBeginPeriod` to modify scheduler granularity

### Day 19: Improving Audio Synchronization

- Record last play cursor and write cursor
- Define `Win32DebugSyncPlay` to draw it
- Use a while loop to test direct sound audio update frequency

### Day 20: Debugging the Audio Sync

**The audio sync logic is indeed very hard and complicated**.

I didn't take many notes because I was really confused and I didn't understand much.

- Compute audio latency seconds using write cursor - play cursor
- Define `GameGetSoundSamples`

### Day 21: Loading Game Code Dynamically

- Compile win32_handmade and handmade separaely
- Define win32_game_code and
- Put platform debug functions to game memroy
- /LD build dll
- Use /EXPORT linker flags to export dll functions
- We don't need to define `DllMain` entry point in our dll
- extern "C" to prevent name mangling
- Turn off incremental link
- Use `CopyFile` to copy the dll

### Day 22: Instantaneous Live Code Editing

Note: `CopyFile` may fail the first time, We use a while loop to do it. This is debug code so We don't care the performance;

- Use /PDB:name linker options to specify pdb file name
- Add timestamp to output pdb file name
- Delete PDB files and pipe del output to NUL
- Use `FindFirstFile` to get file write time
- Use `CompareFileTime` to compare file time
- Use `GetModuleFileName` to get exe path and use it to build full dll path
- We can use MAX_PATH macro to define length of path buffer

### Day 23: Looped Live Code Editing

- Define `win32_state` to store InputRecordIndex and InputPlayingIndex, we only support one slot now
- Press L to toggle input recording
- Store input and memory into files
- Use a simple jump to test our looped editing
- We can use `WS_EX_TOPMOST` and `WS_EX_LAYERED` to make our window the top most one and has some opacity
- We can do it in `WM_ACTIVATEAPP` message so when the game loses focus it will be transparent

### Day 24: Win32 Platform Layer Cleanup

- Fix the audio bug (I have already fixed that in previous day)
- Change blit mode to 1-to-1 pixles
- Use `%random%` for pdb files
- Change compiler flag `MT` to `MTd`
- Store EXE directory in Win32State and put record input file to build dir
- Use `GetFileAttributeEx` instead of `FindFirstFile` to get last write time of a file

### Day 25: Finishing the Win32 Prototyping Layer

- Use `GetDeviceCaps` to get monitor refresh rate
- Pass `thread_context` from platform to game and from game to platform
- Add mouse info to game_input, using `GetCursorPos`, `ScreenToClient`
- Record mouse buttons using `GetKeyState`
- Define win32_replay_buffer and store game state memory in memory using `CopyMemory` (Storing in disk actually is very fast in my computer, but I am gonna do it anyway)
- I am not gonna do memory mapping, because I think it's unnecessary

### Day 26: Introduction to Game Architecture

Today there isn't any code to write. I am just listening to Casey talking about what a good game architecture looks like.

In Casey's view, game architect is like a **Urban Planner**. Their job are organizing things roughly instead of planning things carefully. I can't agree more.

### Day 27: Exploration-based Architecture

- Add `SecondsToAdvanceOverUpdate` to game_input
- Remove debug code
- Turn off warning C4505, it's annoying. We are gonna have unreferenced local functions.
- Target resolution: 960 x 540 x 30hz
- Define `DrawRectangle`

### Day 28: Drawing a Tile Map

- Use floating point to store colors, because it will make it a lot more eaiser when we have to do some math about colors
- Draw a simple tile map
- Draw a simple player, keep in mind that player's moving should consider the time delta. Otherwise it will move fast if we run at a higher FPS.
