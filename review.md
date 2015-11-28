# Handmade Hero Review

[TOC]

![](./home.png)

## First Things First

- This is my personal learning notes of [Handmade Hero](https://handmadehero.org/)
- If you think **writing a professional-quality game from scratch on your own (no engine no library)** is interesting and challenging, I highly recommend this project. In my opinion, it's the best I can find
- Deepest thanks to *Casey Muratori* for this excellent project, it's really helpful



## Roadmap

### Day 1 - 35

I forgot to write ...

### Day 36 -- Loading BMPs

- Go between different z layers
- TileRelX -> OffsetX
- Load BMP File

### Day 37 -- Basic Bitmap Rendering

- Make a structured BMP to figure out BMP file format, 0xRRGGBBAA, bottom up
- Transform BMP Pixel
- write a function DrawBitmap

### Day 38 -- Basic Linear Bitmap Blending

- 7 taskbar tweaker
- BitScanForward, windows intrisics
- Draw Picture with Alpha Channel

### Day 39 -- Basic Bitmap Rendering Cleanup

- struct hero_bitmaps 
- AlignX and AlignY
- CameraP
- SourceOffsetX and SourceOffsetY -> change DrawBitmap based on clipping
- About PDB file, lock.tmp

### Day 40 -- Cursor Hiding and Fullscreen Support

- remove tSine in win32_sound_buffer
- Visual Studio Spy++
- LoadCursor, SetCursor
- [Fullscreen](http://blogs.msdn.com/b/oldnewthing/archive/2010/04/12/9994016.aspx)
- Windows Expert: [Raymond Chen](http://blogs.msdn.com/b/oldnewthing/)

### Day 41 -- Overview of the Types of Math Used in Game

- Nothing about code

### Day 42 -- Basic 2D Vectors

- struct v2
- c++ operator overload, + - * for v2

### Day 43 -- The Equations of Motion

- GameState->dPlayerP, means velocity
- ddPlayer, means acceleration
- ODE, add friction to velocity

### Day 44 -- Reflecting Vectors

- Inner Product
- Reflecting the velocity when hitting the wall

### Day 45 — Geometric vs. Temporal Movement Search

- Search in time VS. Search in position in terms of collision detection
- Need to watch day 46 to figure out what the geometric search means

### Day 46 - Basic Multiplayer Support

- Fix previous bug, controller->IsAnalog
- struct Entity to store character position and velocity
- Initialize, initialise player position (AbsTileX, AbsTileY, OffsetX, OffsetY)
- CameraFollowingEntity
- PlayerIndexForController
- fabs

### Day 47 - Vector Lengths

- Normalize ddP's length
- sqrtf
- Search in time rather than search in position cause search in position is so complicated



## Windows Programming

### Misc

- `extern c` tell compiler not to do “name mangling”
- `/LD` make MSVC compiler produce a DLL 
- `/fp:fast` make MSVC compiler use fast floating point calculation
- `#pragma pack(push, 1) and #pragma pack(pop)` resolve struct pack problem

### Functions

- GetDeviceCaps, get monitor refresh rate
- GetCursorPos
- ScreenToClient
- GetKeyboardState
- SetFilePointer
- CopyMemory
- MapViewOfFile, map memory and file
- LARGE_INTEGER macro
- _BitScanForward



### Batch

- `findstr -s -n -i -l "content" "*.cpp"`, search str in files



## Other

- [Intel Intrinsics Guide](https://software.intel.com/sites/landingpage/IntrinsicsGuide/)
  
  ​