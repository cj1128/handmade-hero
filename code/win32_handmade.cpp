#include <windows.h>

LRESULT CALLBACK MainWindowCallback(
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
      static DWORD Color = WHITENESS;
      PatBlt(DeviceContext, X, Y, Width, Height, Color);
      if(Color == WHITENESS) {
        Color = BLACKNESS;
      } else {
        Color = WHITENESS;
      }
      EndPaint(Window, &PaintStruct);
    } break;

    case WM_SIZE:
    {
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
  WindowClass.lpfnWndProc = MainWindowCallback;
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
      while(1) {
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
