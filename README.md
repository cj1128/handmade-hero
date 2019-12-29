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

Windows 10 with Visual Studio 2019 community version and Sublime Text 3.

## Windows Programming

### Command Prompt

- `dir /s [keyword]`: search files
- `findstr -s -n -i -l [keyword]`: find strings

### Win32 API

- `WS_EX_TOPMOST`: make window in front of others
- `WS_EX_LAYERED` and `SetLayeredWindowAttributes `: change window alpha

### Visual Studio

- `Spy++`: inspect windows and messages

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

NOTE: `CopyFile` may fail the first time, We use a while loop to do it. This is debug code so We don't care the performance;

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

### Day 28: Drawing a Tilemap

- Use floating point to store colors, because it will make it a lot more eaiser when we have to do some math about colors
- Draw a simple tilemap
- Draw a simple player, keep in mind that player's moving should consider the time delta. Otherwise it will move fast if we run at a higher FPS.

### Day 29: Basic Tilemap Collision Checking

- Using `PatBlt` to clear screen when display our buffer
- We should only clear the four gutters otherwise there will be some flashing
- Implement a simple collision check
- Seperate the header file into handmade.h and handmade_platform.h

### Day 30: Moving Between Tilemaps

- Define four tilemaps, and notice that in C the two dimension array is Y first and X last
- Define `canonicol_position` and `raw_postion`
- Implement moving between tilemaps

### Day 31: Tilemap Coordinate Systems

- NOTE: basically any CPU we are gonna target at has SSE2
- Define `handmade_intrinsic.h`
- Define `TileSizeInMeters` and `TileSizeInPixels`
- Optimization switches: `/O2 /Oi /fp:fast`
- PLAN: pack tilemap index and tile index into a single 32-bit integer
- PLAN: convert TileRelX and TileRelY to resolution independent world units
- RESOURCE: Intel Intrinsics Guide, https://software.intel.com/sites/landingpage/IntrinsicsGuide/

### Day 32: Unified Position Representation

- Remove `raw_position`
- Add `canonical_postion PlayerPos` to game state
- Define `RecononicalizePosition`
- Use meters instead of pixels as units

### Day 33: Virtualized Tilemaps

- Rename `canonical_position` to `world_position`
- Make Y axis go upward
- RESOURCE: a great book about typology, Galois' Dream: Group Theory and Differential Equations
- Remove TileMapX and TileMapY
- Define `tile_map_position`
- 24-bit for tilemap and 8-bit for tiles
- Implement a simple scroll so the guy can move

### Day 34: Tilemap Memory

- Implement smooth scrolling
- Implement a way to speed the guy up
- Make `TileRelX` and `TileRelY` relative to center of tile
- Create `handmade_tile.h` and `handmade_tile.cpp`
- Rename `tile_map` to `tile_chunk` and extract everything from `world` to `tile_map`, now tilemap means the whole map
- Define `memory_arena` and `PushSize`, `PushArray` and create tile chunks programmatically

### Day 35: Basic Sparse Tilemap Storage

- Make tile size small so we can see more chunks
- Remove `TileSizeInPixels` and `MetersToPixels` from `tile_map`
- Use random.org to generate some random numbers and use them to generate screen randomly
- Generate doors based on our choice
- Allocate space for tiles only when we access
- Add Z index to tilemap

### Day 36: Loading BMPs

- FUN: Windows can't render BMPs correct. This is very amusing, because they are the guys who invented BMP.
- Make player go up and down. I already implemented this function in the previous day, but I need to implement it in a new way: when the player moves to the stair, it goes automatically, no need to push any button.
- Rename `TileRelX` and `TileRelY` to `OffsetX` and `OffsetY`
- Define `bitmap_header` and parse bitmap. We have to use `#pragma pack(push, 1) and #pragma pack(pop)` to make vs pack our struct correctly

### Day 37: Basic Bitmap Rendering

- Design a very specific BMP to help debug our rendering. This is a very clever method.
- I find that my structured_art.bmp has different byte order from casey's. It turns out that BMP has something called RedMask, GreenMask, BlueMask and AlphaMask.
- BMP byte order: should determined by masks
- Render background bmp
- Define `loaded_bitmap` to pack all things up
- Define `DrawBitmap`

### Day 38: Basic Linear Bitmap Blending

- Define `FindLeastSignificantSetBit` and `bit_scan_result` in intrinsics
- Define `COMPILER_MSVC` and `COMPILER_LLVM` macro variables
- Use `_BitScanForward` MSVC compiler intrinsic when under windows
- Implement a simple linear alpha blending
- Assert compression mode when loading BMP

### Day 39: Basic Bitmap Rendering Cleanup

- Load hero bitmaps for four directions
- Change hero direction when moves
- Align hero bitmaps with real position
- Replace camera scrolling with fixed camera
- Move camera when player moves
- Fix clipping problem in our bitmap drawing
- Check frame rate
- Fix msvc pdb problem when hot reloading by create a lock file

### Day 40: Cursor Hiding and Fullscreen

- Write a `static_check` bat file to make sure we never type `static`
- Set a default cursor style using `LoadCursor`
- Hide cursor by responding `WM_SETCURSOR` message with `SetCursor(0)` in production build
- RESOURCE: How do I switch a window between normal and fullscreen? https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
- Implement full screen toggling
- Do fullscreen rendering in fullscreen mode

### Day 41: Overview of the Types of Math Used in Games

- Math we are gonna need:
  - Arithmetic
  - Algebra
  - Euclidean Geometry
  - Trigonometry
  - Arithmetic
  - Calculus
  - Linear Algebra
  - Partial Differential Equation
  - Ordinary Differential Equation
  - Complex Numbers
  - Non-Euclidean Geometry
  - Topology
  - Minkowski Algebra
  - Control Theory
  - Interval Arithmetic
  - Graph Theory
  - Operations Research
  - Probability and Statistics
  - Cryptography / Pseudo Number Generator

### Day 42: Basic 2D Vectors

- Fix diagonal movement problem
- Define `v2` and plus operator, minus operator and unary minus operator in `handmade_math.h`
- Use v2 instead of x and y

### Day 43: The Equations of Motion

- Add `dPlayerP` to game state. This is the speed of the guy.
- Add a back force based on player's speed

### Day 44: Reflecting Vectors

- Implement inner product for vectors
- Reflect speed when player hits the wall (or make the speed align the wall). This can be implemented by a clever verctor math (v' = v - 2 * Inner(v, r) * r). r means the vector of the reflecting direction. For bottom wall, r is '(0, 1)'.

### Day 45: Geometric vs. Temporal Movement Search

- OPINION: Search in p(position) is way better than searing in t(time)
- Part of new collision detection algorithm
- There is no code today. I will write the new collision detection algorithm when it's complete.

### Day 46: Basic Multiplayer Support

- We have a severe bug! Player has been moved multiple times!
- Define `entity` struct. Add `Entities`, `EntityCount`, `PlayerIndexForController` and `CameraFollowingEntityIndex` to game state
- Support as many players as our controllers in game state
- Implement `RotateLeft` and `RotateRight` intrincsics using `_rotl` and `_rotr`

### Day 47: Vector Lengths

- Define `LengthSq`, `SquareRoot` to fix diagonal movement problem
- We will *USE* search in t instead of search in p, because the later will have to build the search space and this is a complicated thing to do
- Part of new collision detection algorithm

### Day 48: Line Segment Intersection Collision

- Implement the new collistion test
- Add a `tEpsilon` to tolerate floating point problem

### Day 49: Debugging Canonical Coordinates

- Add an `Offset` method to manipulate tile map position and auto recononicalize
- Maybe we shouldn't make the world toroidal, since it adds much complexity
