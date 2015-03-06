#include <windows.h>

#define local_persist static
#define internal static
#define global_variable static

global_variable bool Running;
global_variable BITMAPINFO BitmapInfo;
global_variable HBITMAP BitmapHandle;
global_variable HDC BitmapDeviceHandle;
global_variable void * BitmapMemory;

internal void Win32UpdateWindow(HDC DeviceContext,int X, int Y, int Width, int Height)
{
  StretchDIBits(DeviceContext,
    X,Y,Width,Height,
    X,Y,Width,Height,
    BitmapMemory,
    &BitmapInfo,
    DIB_RGB_COLORS,SRCCOPY);
}

internal void Win32ResizeDIBSection(int Width, int Height)
{
  if(BitmapHandle){
    DeleteObject(BitmapHandle);
  }

  if( BitmapDeviceHandle == 0)
  {
    BitmapDeviceHandle = CreateCompatibleDC(0);
  }

  BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biWidth = Width;
  BitmapInfo.bmiHeader.biHeight = Height;
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32;
  BitmapInfo.bmiHeader.biCompression = BI_RGB;

  BitmapHandle  = CreateDIBSection(
    BitmapDeviceHandle,
    &BitmapInfo,
    DIB_RGB_COLORS,
    &BitmapMemory,
    0,0);
}

LRESULT CALLBACK MainWindowCallback(HWND Window,
                            UINT Message,
                            WPARAM wParam,
                            LPARAM lParam)
{
  LRESULT Result = 0;
  switch(Message){
    case WM_SIZE:
    {
      RECT ClientRect ;
      GetClientRect(Window, &ClientRect);
      int Width = ClientRect.right - ClientRect.left;
      int Height = ClientRect.bottom - ClientRect.top;
      Win32ResizeDIBSection(Width, Height);
    } break;

    case WM_DESTROY:
    {
      Running = false;
    } break;

    case WM_CLOSE:
    {
      Running = false;
    } break;

    case WM_ACTIVATEAPP:
    {
      OutputDebugStringA("ACTIVATEAPP\n");
    } break;

    case WM_PAINT:
    {
      PAINTSTRUCT Paint;
      HDC DeviceContext = BeginPaint(Window,&Paint);
      int X = Paint.rcPaint.left;
      int Y = Paint.rcPaint.top;
      int Width = Paint.rcPaint.right - Paint.rcPaint.left;
      int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
      Win32UpdateWindow(DeviceContext, X, Y, Width, Height);
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
      Running = true;
      MSG Message;
      while(Running){
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

