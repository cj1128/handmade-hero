<div align="center">
  <h1>
    <a href="https://handmadehero.org/">Handmade Hero</a>
  </h1>

  <img src="./home.png">
</div>

## Intro

- This is my personal study notes of [Handmade Hero](https://handmadehero.org).
- If you think **writing a professional-quality game from scratch on your own (no engine no library)** is interesting and challenging, I highly recommend this project. In my opinion, it's the best I can find.
- Deepest thanks to *Casey Muratori* for this excellent project. It's really helpful.

## Env

Windows 10 with Visual Studio 2019 and Sublime Text 3.

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
