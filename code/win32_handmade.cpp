#include <windows.h>
#include <stdint.h>

#define local_persist static
#define internal static
#define global_variable static

global_variable bool Running;
global_variable BITMAPINFO BitmapInfo;
global_variable void * BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;


internal void
RenderWeirdGradient(int BlueOffset, int GreenOffset)
{
  int Width = BitmapWidth;
  int Height = BitmapHeight;

  int Pitch = Width * BytesPerPixel;
  uint8_t *Row = (uint8_t *)BitmapMemory;

  for(int Y = 0;Y < BitmapHeight; Y++)
  {
    uint32_t *Pixel = (uint32_t *)Row;
    for(int X = 0; X < BitmapWidth; X++)
    {
      // Memory Order: BB GG RR XX
      // 0xXXBBGGRR
      uint8_t Blue = X + BlueOffset;
      uint8_t Green = Y + GreenOffset;

      *Pixel++ = ((Green << 8) | Blue );

    }
    Row += Pitch;
  }
}



internal void Win32UpdateWindow(HDC DeviceContext,int WindowWidth, int WindowHeight)
{
  StretchDIBits(DeviceContext,
    0,0,BitmapWidth,BitmapHeight,
    0,0,WindowWidth,WindowHeight,
    BitmapMemory,
    &BitmapInfo,
    DIB_RGB_COLORS,SRCCOPY);
}

internal void Win32ResizeDIBSection(int Width, int Height)
{
  if(BitmapMemory){
    VirtualFree(BitmapMemory, 0, MEM_RELEASE);
  }
  BitmapWidth = Width;
  BitmapHeight = Height;

  BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biWidth = Width;
  BitmapInfo.bmiHeader.biHeight = Height;
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32;
  BitmapInfo.bmiHeader.biCompression = BI_RGB;

  int BitmapMemorysize = Width * Height * BytesPerPixel;
  BitmapMemory = VirtualAlloc(0, BitmapMemorysize, MEM_COMMIT, PAGE_READWRITE);
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

      RECT ClientRect;
      GetClientRect(Window, &ClientRect);

      Win32UpdateWindow(DeviceContext, ClientRect.right - ClientRect.left, ClientRect.bottom - ClientRect.top);
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
      int XOffset = 0;
      int YOffset = 0;

      while(Running)
      {
        MSG Message;
        while( PeekMessage(&Message,WindowHandle,0,0, PM_REMOVE) )
        {
          if(Message.message == WM_QUIT){
            Running = false;
          }
          TranslateMessage(&Message);
          DispatchMessage(&Message);
        }

        RenderWeirdGradient(XOffset, YOffset);

        HDC DeviceContext = GetDC(WindowHandle);

        RECT ClientRect;
        GetClientRect(WindowHandle, &ClientRect);

        int WindowWidth = ClientRect.right - ClientRect.left;
        int WindowHeight = ClientRect.bottom - ClientRect.top;

        Win32UpdateWindow(DeviceContext, WindowWidth,WindowHeight);
        ReleaseDC(WindowHandle, DeviceContext);

        XOffset++;
        YOffset += 2;
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


