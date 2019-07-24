#include <windows.h>
#include <stdint.h>

#define i32 int32_t

#define global_variable static
#define interval static

global_variable bool Running = true;

global_variable void *BitmapMemory;
global_variable BITMAPINFO BitmapInfo;
global_variable HDC BitmapDeviceContext;
global_variable HBITMAP BitmapHandle;

void Win32ResizeDIBSection(int Width, int Height) {
  if(BitmapHandle) {
    DeleteObject(BitmapHandle);
  }
  if(!BitmapDeviceContext) {
    BitmapDeviceContext = CreateCompatibleDC(0);
  }

  BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biWidth = Width;
  BitmapInfo.bmiHeader.biHeight = Height;
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32;
  BitmapInfo.bmiHeader.biCompression = BI_RGB;

  BitmapHandle = CreateDIBSection(
    BitmapDeviceContext,
    &BitmapInfo,
    DIB_RGB_COLORS,
    &BitmapMemory,
    0,
    0
  );

  // clear it to white
  i32 *p = (i32 *)BitmapMemory;
  for(int i = 0; i < Width * Height; i++) {
    *p++ = 0xffffffff;
  }
}

void Win32UpdateWindow(HDC DeviceContext, int X, int Y, int Width, int Height) {
  StretchDIBits(
    DeviceContext,
    X, Y, Width, Height,
    X, Y, Width, Height,
    BitmapMemory,
    &BitmapInfo,
    DIB_RGB_COLORS,
    SRCCOPY
  );
}

LRESULT CALLBACK Win32MainWindowCallback(
  HWND Window,
  UINT Message,
  WPARAM WParam,
  LPARAM LParam
) {
  LRESULT Result = 0;
  switch(Message) {
    case WM_ACTIVATEAPP:
    {
      OutputDebugStringA("WM_ACTIVATEAPP\n");
    } break;

    case WM_CLOSE:
    {
      Running = false;
      OutputDebugStringA("WM_CLOSE\n");
    } break;

    case WM_DESTROY:
    {
      OutputDebugStringA("WM_DESTROY\n");
    } break;

    case WM_PAINT:
    {
      PAINTSTRUCT PaintStruct = {};
      HDC DeviceContext = BeginPaint(Window, &PaintStruct);
      int X = PaintStruct.rcPaint.left;
      int Y = PaintStruct.rcPaint.top;
      int Width = PaintStruct.rcPaint.right - PaintStruct.rcPaint.left;
      int Height = PaintStruct.rcPaint.bottom - PaintStruct.rcPaint.top;
      Win32UpdateWindow(DeviceContext, X, Y, Width, Height);
      EndPaint(Window, &PaintStruct);
    } break;

    case WM_SIZE:
    {
      RECT ClientRect;
      GetClientRect(Window, &ClientRect);
      int Width = ClientRect.right - ClientRect.left;
      int Height = ClientRect.bottom - ClientRect.top;
      Win32ResizeDIBSection(Width, Height);
      OutputDebugStringA("WM_SIZE\n");
    } break;

    default:
    {
      Result = DefWindowProc(Window, Message, WParam, LParam);
    } break;
  }

  return Result;
}

int CALLBACK WinMain(
  HINSTANCE Instance,
  HINSTANCE PrevInstance,
  LPSTR     CmdLine,
  int       ShowCmd
) {
  WNDCLASS WindowClass = {};

  WindowClass.style = CS_HREDRAW | CS_VREDRAW;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = Instance;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";

  if(RegisterClass(&WindowClass)) {
    HWND WindowHandle = CreateWindowEx(
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

    if(WindowHandle) {
      while(Running) {
        MSG Message = {};
        BOOL MessageResult = GetMessage(&Message, 0, 0, 0);

        if(MessageResult > 0) {
          TranslateMessage(&Message);
          DispatchMessage(&Message);
        } else {
          break;
        }
      }
    } else {
      // TODO: Logging
    }
  } else {
    // TDOO: Logging
  }

	return 0;
}
