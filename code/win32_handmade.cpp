#include <windows.h>

LRESULT CALLBACK MainWindowCallback(HWND Window,
                            UINT Message,
                            WPARAM wParam,
                            LPARAM lParam)
{
  LRESULT Result = 0;
  switch(Message){
    case WM_SIZE:
    {
      OutputDebugStringA("SIZE\n");
    } break;

    case WM_DESTROY:
    {
      OutputDebugStringA("DESTROY\n");
    } break;

    case WM_CLOSE:
    {
      OutputDebugStringA("CLOSE\n");
    } break;

    case WM_ACTIVATEAPP:
    {
      OutputDebugStringA("ACTIVATEAPP\n");
    } break;

    case WM_PAINT:
    {
      PAINTSTRUCT Paint;
      HDC DeviceContext = BeginPaint(Window,&Paint);
      int Width = Paint.rcPaint.right - Paint.rcPaint.left;
      int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
      static DWORD Color = WHITENESS;
      PatBlt(
        DeviceContext,
        Paint.rcPaint.left,
        Paint.rcPaint.top,
        Width,
        Height,
        Color
      );
      if(Color == WHITENESS){
        Color = BLACKNESS;
      }
      else{
        Color = WHITENESS;
      }
      EndPaint(Window,&Paint);
    } break;

    default:
    {
      OutputDebugStringA("DEFAULT\n");
      Result = DefWindowProc(Window,Message,wParam,lParam);
    }
  }

  return Result;
}

int CALLBACK
WinMain(HINSTANCE Instance,
	HINSTANCE PrevInstance,
	LPSTR CmdLine,
	int CmdShow)
{
  WNDCLASS WindowClass = {};

  //TODO: check if owndc,hredraw,vredraw matter
  WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
  WindowClass.lpfnWndProc = MainWindowCallback;
  WindowClass.hInstance = Instance;
  // HICON     hIcon;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";

  if(RegisterClass(&WindowClass))
  {
    HWND WindowHandle =  CreateWindowEx(
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
    if(WindowHandle){
      MSG Message;
      for(;;){
        if(GetMessage(&Message,WindowHandle,0,0) > 0){
          TranslateMessage(&Message);
          DispatchMessage(&Message);
        }
        else{
          break;
        }
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

