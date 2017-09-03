// bcoder project; code editor mainly for c/c++;
// author: Brajan Bartoszewicz
#include <windows.h>

#include <GL/gl.h>

#include "bcoder.h"
#include "bcoder_string.h"
#include "bcoder_config.h"

// cpps
#include "bcoder.cpp"
#include "bcoder_string.cpp"
#include "bcoder_lexer.cpp"

#if BCODER_RENDERER_OPENGL == 1
#include "bcoder_renderer_opengl.cpp"
#elif BCODER_RENDERER_OS == 1
#include "bcoder_renderer_win32.cpp"
#endif

struct win32_window_dimensions {
  int Width;
  int Height;
};

struct win32_offscreen_buffer {
  BITMAPINFO BitmapInfo;
  bitmap Bitmap;
};

struct win32_opengl_context {
  HDC DeviceContext;
  HGLRC ContextHandle;
  HWND WindowHandle;
};

global_variable bool IsExitRequested = false;
global_variable HWND Win32Window = 0;
global_variable win32_opengl_context OpenGLContext;

internal u8 *
Win32GetFileContent(const char *FileName) {
  HANDLE File = 0;
  ULONG NumberRead = 0;
  u8 *Data = 0;
  u32 FileSize = 0;

  File = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
  if (File == INVALID_HANDLE_VALUE) {
    Assert(false);
    return 0;
  }

  FileSize = GetFileSize(File, 0);
  Data = (u8 *)VirtualAlloc(0, FileSize, MEM_COMMIT, PAGE_READWRITE);
  ReadFile(File, Data, FileSize, &NumberRead, 0);
  
  CloseHandle(File);
  return Data;
}

internal void
Win32GetGlyphBitmap(const char *Filename, const char *FontName,
                    int *Width, int *Height, int *OffsetX, int *OffsetY,
                    char Codepoint, int FontSize,
                    u32 *OutputBitmap,
                    int OutputWidth, int OutputHeight) {
  Assert(Width != 0);
  Assert(Height != 0);
  Assert(OffsetX != 0);
  Assert(OffsetY != 0);
  Assert(OutputBitmap != 0);
  Assert(OutputWidth > 0);
  Assert(OutputHeight > 0);

  static void *Bits = 0;
  static HDC DeviceContext = 0;

  if (!DeviceContext) {
    AddFontResourceExA(Filename, FR_PRIVATE, 0);
    int Height = FontSize; 
    
    HFONT Font = CreateFontA(Height, 0, 0, 0,
                             FW_NORMAL,
                             FALSE,
                             FALSE,
                             FALSE,
                             DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS,
                             //CLEARTYPE_QUALITY,
                             ANTIALIASED_QUALITY,
                             DEFAULT_PITCH | FF_DONTCARE,
                             FontName);
    //HFONT Font = CreateFont(FontSize, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FontName);

#if 1
    DeviceContext = CreateCompatibleDC(GetDC(0));
    
    BITMAPINFO BitmapInfo = {};
    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = OutputWidth;
    BitmapInfo.bmiHeader.biHeight = OutputHeight;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biSizeImage = 0;
    BitmapInfo.bmiHeader.biXPelsPerMeter = 0;
    BitmapInfo.bmiHeader.biYPelsPerMeter = 0;
    BitmapInfo.bmiHeader.biClrUsed = 0;
    BitmapInfo.bmiHeader.biClrImportant = 0;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;
    
    HBITMAP Bitmap = CreateDIBSection(DeviceContext, &BitmapInfo, DIB_RGB_COLORS, &Bits, 0, 0);
    SelectObject(DeviceContext, Bitmap);
    SelectObject(DeviceContext, Font);
    SetBkColor(DeviceContext, RGB(255, 255, 255));

    TEXTMETRIC TextMetric;
    GetTextMetrics(DeviceContext, &TextMetric);
#endif
#if 0
    DeviceContext = CreateCompatibleDC(0);
    HBITMAP Bitmap = CreateCompatibleBitmap(DeviceContext, OutputWidth, OutputHeight);
    SelectObject(DeviceContext, Bitmap);
    SelectObject(DeviceContext, Font);
    SetBkColor(DeviceContext, RGB(0, 0, 0));

    TEXTMETRIC TextMetric;
    GetTextMetrics(DeviceContext, &TextMetric);
#endif
  }

  wchar_t CheesePoint = (wchar_t)Codepoint;

  SIZE Size;
  GetTextExtentPoint32W(DeviceContext, &CheesePoint, 1, &Size);
  
  *Width = Size.cx;
  *Height = Size.cy;
  *OffsetX = 0;
  *OffsetY = 0;

  //PatBlt(DeviceContext, 0, 0, *Width, *Height, BLACKNESS);
  //SetBkMode(DeviceContext, TRANSPARENT);
  SetTextColor(DeviceContext, RGB(0, 0, 0));

  TextOutW(DeviceContext, 0, 0, &CheesePoint, 1);

  for (int Y = 0; Y < *Height; ++Y) {
    for (int X = 0; X < *Width; ++X) {
      COLORREF Pixel = GetPixel(DeviceContext, X, Y);
      u8 Gray = (u8)(Pixel & 0xFF);
      u8 Alpha = 0xFF;
      OutputBitmap[X + OutputWidth * Y] = ((GetRValue(Pixel) << 0) |
                                           (GetGValue(Pixel) << 8) |
                                           (GetBValue(Pixel) << 16) |
                                           (Alpha << 24));
    }
  }
}

internal void
Win32LoadFont(u32 FontSize, const char *Filename, u32 AtlasWidth, u32 AtlasHeight) {
#if BCODER_RENDERER_OPENGL == 1
  bitmap Atlas;
  Atlas.Width = AtlasWidth;
  Atlas.Height = AtlasHeight;
  Atlas.BytesPerPixel = 4;
  u32 AtlasMemorySize = Atlas.Width * Atlas.Height * Atlas.BytesPerPixel;
  Atlas.Memory = VirtualAlloc(0, AtlasMemorySize, MEM_COMMIT, PAGE_READWRITE);

  // TODO(Brajan): fill atlas
  u8 *FontFileContent = Win32GetFileContent(Filename);

  if (FontFileContent == 0) {
    Assert(false);
  }

  u8 *MonoBitmap = (u8 *)VirtualAlloc(0, AtlasWidth * AtlasHeight, MEM_COMMIT, PAGE_READWRITE);
  u32 *GlyphBitmap = (u32 *)VirtualAlloc(0, Atlas.Width * Atlas.Height * 4, MEM_COMMIT, PAGE_READWRITE);

  int PackingPosX = 0;
  int PackingPosY = 0;

  for (int Index = 0; Index < GlyphsNumber; ++Index) {
    int Width = 0;
    int Height = 0;
    int OffsetX = 0;
    int OffsetY = 0;

    Win32GetGlyphBitmap("c:/windows/fonts/LiberationMono-Regular.ttf", "Liberation Mono",
    //Win32GetGlyphBitmap("c:/windows/fonts/Arial.ttf", "Arial",
                        &Width, &Height, &OffsetX, &OffsetY,
                        StartCharacter + Index, FontSize,
                        GlyphBitmap,
                        Atlas.Width, Atlas.Height);
    
    if (PackingPosX >= Atlas.Width) {
      PackingPosX = 0;
      PackingPosY += FontSize;
    }

    u32 *AtlasBitmap = (u32 *)Atlas.Memory;
    for (int Y = 0; Y < Height; ++Y) {
      for (int X = 0; X < Width; ++X) {
        AtlasBitmap[(X + PackingPosX) + Atlas.Width * (Y + PackingPosY)] = GlyphBitmap[X + Atlas.Width * Y];

        //MonoBitmap[(X + PackingPosX) + Atlas.Width * (Y + PackingPosY)] = GlyphMonoBitmap[X + Atlas.Width * Y];   
      }
    }

    glyph *Glyph = &BCoderState.Font.Glyphs[Index];
    Glyph->X0 = PackingPosX;
    Glyph->Y0 = PackingPosY;
    Glyph->X1 = PackingPosX + Width;
    Glyph->Y1 = PackingPosY + Height;
    Glyph->Width = Width;
    Glyph->Height = Height;
    Glyph->XAdvance = Width;
    Glyph->XOffset = OffsetX;
    Glyph->YOffset = OffsetY;

    PackingPosX += Width;
  }

  VirtualFree(GlyphBitmap, 0, MEM_RELEASE);

  /*u8 *Source = MonoBitmap;
  u8 *Row = (u8 *)Atlas.Memory;
  for (int Y = 0; Y < Atlas.Height; ++Y) {
    u32 *Pixel = (u32 *)Row;
    for (int X = 0; X < Atlas.Width; ++X) {
      u8 Alpha = *Source++;
      *Pixel++ = ((Alpha << 24) |
                  (Alpha << 16) |
                  (Alpha << 8)  |
                  (Alpha << 0));
    }
    Row += Atlas.Width * Atlas.BytesPerPixel;
  }*/

  BCoderState.Font.Atlas = Atlas;
  BCoderState.Font.Size = FontSize;
  BCoderState.LineHeight = FontSize;

  VirtualFree(FontFileContent, 0, MEM_RELEASE);
  VirtualFree(MonoBitmap, 0, MEM_RELEASE);
#elif BCODER_RENDERER_OS == 1
  SetFontWin32(FontSize, Filename);
#endif
}

internal bool
Win32ReadFileIntoBuffer(const char *FileName, buffer *Buffer) {
  HANDLE File = 0;
  u32 FileSize = 0;
  char *Data = 0;
  ULONG NumberRead = 0;

  ZeroMemory(Buffer, sizeof(buffer));
  
  File = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
  if (File == INVALID_HANDLE_VALUE) {
    return false;
  }

  FileSize = GetFileSize(File, 0);
  Data = (char *)VirtualAlloc(0, FileSize, MEM_COMMIT, PAGE_READWRITE);
  Buffer->Memory = VirtualAlloc(0, Megabytes(1), MEM_COMMIT, PAGE_READWRITE);
  Buffer->LineEnding = LineEnding_Unix;

  ReadFile(File, Data, FileSize, &NumberRead, 0);
  for (u32 Index = 0;
       Index < FileSize;
       ++Index) {
    // NOTE(Brajan): I do not want to have this trash in buffers.
    if (Data[Index] == '\r') {
      Buffer->LineEnding = LineEnding_Dos;
    }
    PushCharacterToBuffer(Buffer, Data[Index]); 
  }
  VirtualFree(Data, 0, MEM_RELEASE);
  CloseHandle(File);

  Buffer->Flags |= BufferFlag_Saved;
  Buffer->Size = Megabytes(1);
  return true;
}

internal void
Win32WriteBufferIntoFile(buffer* Buffer, const char *FileName) {
  if (Buffer->Flags & BufferFlag_NotSavable)
    return;

  HANDLE File = CreateFile(FileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  Assert(File != INVALID_HANDLE_VALUE);
  WriteFile(File, Buffer->Memory, Buffer->Length, NULL, 0);
  CloseHandle(File);

  Buffer->Flags |= BufferFlag_Saved;
}

internal buffer
Win32CreateEmptyBuffer(u32 InitialSize) {
  buffer Buffer;
  ZeroMemory(&Buffer, sizeof(Buffer));
  StringCopy(Buffer.Name, "-- Empty --");
  Buffer.Memory = VirtualAlloc(0, InitialSize, MEM_COMMIT, PAGE_READWRITE);
  ZeroMemory(Buffer.Memory, InitialSize);
  return Buffer;
}

internal void
Win32FreeBuffer(buffer *Buffer) {
  Assert(Buffer != 0);
  VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
}

internal void
Win32ShowError(const char *Error) {
  MessageBoxA(NULL, Error, "Fatal error", MB_OK | MB_ICONERROR);
}

internal char *
Win32GetSystemClipboard() {
  OpenClipboard(NULL);
  HANDLE Clipboard = GetClipboardData(CF_TEXT);  
  if (Clipboard != NULL) {
    char *ClipboardPtrData = (char *)GlobalLock(Clipboard);
    u32 ClipboardLength = StringLength(ClipboardPtrData);
    char *Result = (char *)VirtualAlloc(0, ClipboardLength, MEM_COMMIT, PAGE_READWRITE);

    ZeroMemory(Result, ClipboardLength);
    StringCopy(Result, ClipboardPtrData);

    GlobalUnlock(ClipboardPtrData);
    CloseClipboard();
    return Result;
  }
  CloseClipboard();
  return 0; 
}

internal void
Win32SetSystemClipboard(const char *Data, u32 Length) {
  HGLOBAL Memory = GlobalAlloc(GMEM_MOVEABLE, Length + 1);
  char *Dest = (char *)GlobalLock(Memory);
  for (u32 Index = 0;
       Index < Length;
       ++Index) {
    Dest[Index] = Data[Index];
  }
  Dest[Length] = '\0';
  GlobalUnlock(Memory);

  OpenClipboard(NULL);
  EmptyClipboard();
  SetClipboardData(CF_TEXT, Memory);
  CloseClipboard();
}

internal void *
Win32AllocateMemory(u32 Size) {
  return VirtualAlloc(0, Size, MEM_COMMIT, PAGE_READWRITE);
}

internal void
Win32FreeMemory(void *Ptr) {
  VirtualFree(Ptr, 0, MEM_RELEASE);
}

internal u32
Win32GetAsyncKeysState() {
  u32 AsyncKeyFlags = AsyncKey_None;
  if (GetAsyncKeyState(VK_CONTROL))
    AsyncKeyFlags |= AsyncKey_Ctrl;
  if (GetAsyncKeyState(VK_MENU))
    AsyncKeyFlags |= AsyncKey_Alt;
  if (GetAsyncKeyState(VK_LSHIFT))
    AsyncKeyFlags |= AsyncKey_Shift;
  return AsyncKeyFlags;
}

u32
Win32RunBuildFile(const char *Name, char *Buffer, u32 Size, u32 *OutSize) {
  char CmdLine[80];
  wsprintf(CmdLine, "cmd.exe /C %s.bat", Name);

  // TODO(Brajan): check if build file exists

  // NOTE(Brajan): creating pipe for handling stdout of child process
  HANDLE ChildStdOutPipeRead = NULL;
  HANDLE ChildStdOutPipeWrite = NULL;
  
  SECURITY_ATTRIBUTES SecurityAttributes = { 0 };
  SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
  SecurityAttributes.bInheritHandle = TRUE;
  SecurityAttributes.lpSecurityDescriptor = NULL;
  
  if (!CreatePipe(&ChildStdOutPipeRead, &ChildStdOutPipeWrite, 
                  &SecurityAttributes, 0)) {
    OutputDebugStringA("Couldn't create pipe for std out");
    return 1;
  }
  
  if (!SetHandleInformation(ChildStdOutPipeRead, HANDLE_FLAG_INHERIT, 0)) {
    OutputDebugStringA("Couldn't set handle information");
    return 1;
  }

  STARTUPINFO StartupInfo;
  ZeroMemory(&StartupInfo, sizeof(StartupInfo));
  StartupInfo.cb = sizeof(StartupInfo);
  StartupInfo.hStdError = ChildStdOutPipeWrite;
  StartupInfo.hStdOutput = ChildStdOutPipeWrite;
  StartupInfo.hStdInput = NULL;
  StartupInfo.dwFlags |= STARTF_USESTDHANDLES;

  PROCESS_INFORMATION ProcessInfo;
  
  BOOL Success = CreateProcessA(NULL, 
                                CmdLine,
                                NULL,
                                NULL,
                                true,
                                CREATE_NO_WINDOW,
                                NULL,
                                NULL,
                                &StartupInfo,
                                &ProcessInfo);
  if (!Success) {
    return 1;
  }
  
  WaitForSingleObject(ProcessInfo.hProcess, INFINITE);

  CloseHandle(ProcessInfo.hProcess);
  CloseHandle(ProcessInfo.hThread);
  CloseHandle(ChildStdOutPipeWrite);
  
  DWORD Read;
  u32 TotalRead = 0;
  Success = FALSE;
  ZeroMemory(Buffer, Size);

  char TempBuffer[4096] = { "" };

  for (;;) {
    Success = ReadFile(ChildStdOutPipeRead, TempBuffer, 4096, &Read, NULL);
    if (!Success || Read == 0) {
      break;
    }
    
    for (u32 Index = 0;
         Index < Read;
         ++Index) {
      if (TempBuffer[Index] == '\r')
        continue;
      Buffer[TotalRead++] = TempBuffer[Index];
    }
  }

  if (OutSize) {
    *OutSize = TotalRead;
  }

  CloseHandle(ChildStdOutPipeRead);
  return 0;
}

internal win32_window_dimensions
Win32GetWindowDimensions(HWND Window) {
  Assert(Window != 0);

  RECT Rect = {};
  win32_window_dimensions Dimensions;   
  GetClientRect(Window, &Rect);
  Dimensions.Width = Rect.right - Rect.left;
  Dimensions.Height = Rect.bottom - Rect.top;
  return Dimensions;
}

internal void
Win32UpdatePanels() {
#if BCODER_RENDERER_OPENGL == 1
#elif BCODER_RENDERER_OS == 1
  RecreatePanelsBitmapsWin32();
#endif
}

internal LRESULT CALLBACK
WindowCallback(HWND Handle, UINT Message, WPARAM WParam, LPARAM LParam) {
  LRESULT Result = 0;

  switch (Message) {
    case WM_DESTROY: {
      IsExitRequested = true;
    } break;

    case WM_CLOSE: {
      IsExitRequested = true;
    } break;
    
    case WM_SIZE: {
      win32_window_dimensions WindowDimensions = Win32GetWindowDimensions(Handle);
      BCoderState.WindowWidth = WindowDimensions.Width;
      BCoderState.WindowHeight = WindowDimensions.Height;

#if BCODER_RENDERER_OPENGL == 1
#elif BCODER_RENDERER_OS == 1
      HDC DeviceContext = GetDC(Handle);
      WindowResizedWin32(DeviceContext);
      ReleaseDC(Handle, DeviceContext);
#endif

      Win32UpdatePanels();
    } break;

#if BCODER_RENDERER_OS == 1
    case WM_PAINT: {
      PAINTSTRUCT PaintStruct;
      HDC DeviceContext;
      DeviceContext = BeginPaint(Handle, &PaintStruct);
      u32 X = PaintStruct.rcPaint.left;
      u32 Y = PaintStruct.rcPaint.top;
      u32 Width = PaintStruct.rcPaint.right - PaintStruct.rcPaint.left;
      u32 Height = PaintStruct.rcPaint.bottom - PaintStruct.rcPaint.top;
      
      DrawBCoderWin32();

      BitBlt(DeviceContext, 0, 0, Width, Height, Renderer.BackbufferDC, 0, 0, SRCCOPY);
      EndPaint(Handle, &PaintStruct); 
    } break;
#endif

    case WM_KEYDOWN: {
      u32 AsyncKeyFlags = AsyncKey_None;
      if (GetAsyncKeyState(VK_CONTROL))
        AsyncKeyFlags |= AsyncKey_Ctrl;
      if (GetAsyncKeyState(VK_MENU))
        AsyncKeyFlags |= AsyncKey_Alt;
      if (GetAsyncKeyState(VK_SHIFT))
        AsyncKeyFlags |= AsyncKey_Shift;

      HandleKeyDown((u32)WParam, AsyncKeyFlags);
    } break;

    case WM_KEYUP: {
    } break;

    case WM_CHAR: {
      ProcessInput((char)WParam);
    } break;

    default: {
      Result = DefWindowProc(Handle, Message, WParam, LParam);
    } break;
  }
  return Result;
}

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR     LpCmdLine,
        int       CmdShow) {
  // setup platform api
  Platform.LoadFont = Win32LoadFont;
  Platform.ReadFileIntoBuffer = Win32ReadFileIntoBuffer;
  Platform.WriteBufferIntoFile = Win32WriteBufferIntoFile;
  Platform.ShowError = Win32ShowError;
  Platform.CreateEmptyBuffer = Win32CreateEmptyBuffer;
  Platform.FreeBuffer = Win32FreeBuffer;
  Platform.GetSystemClipboard = Win32GetSystemClipboard;
  Platform.SetSystemClipboard = Win32SetSystemClipboard;
  Platform.AllocateMemory = Win32AllocateMemory;
  Platform.FreeMemory = Win32FreeMemory;
  Platform.GetAsyncKeysState = Win32GetAsyncKeysState;
  Platform.RunBuildFile = Win32RunBuildFile;
  Platform.UpdatePanels = Win32UpdatePanels;

  // initialize bcoder before creating win32 window
  InitializeBCoder();

  WNDCLASS WindowClass = {};
  WindowClass.style = CS_HREDRAW | CS_VREDRAW;
  WindowClass.lpszClassName = "bcoder_win32";
  WindowClass.hInstance = Instance;
  WindowClass.lpfnWndProc = &WindowCallback;
  WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  WindowClass.hbrBackground = GetSysColorBrush(COLOR_3DFACE);

  RegisterClass(&WindowClass);
    
  Win32Window = CreateWindowEx(0,
                               WindowClass.lpszClassName,
                               "bcoder",
                               WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT, 
                               CW_USEDEFAULT,
                               0,
                               0,
                               Instance,
                               0);

#if BCODER_RENDERER_OPENGL == 1
  OpenGLContext.DeviceContext = GetDC(Win32Window);
  OpenGLContext.WindowHandle = Win32Window;

  PIXELFORMATDESCRIPTOR PixelFormat = {};
  PixelFormat.nSize = sizeof(PixelFormat);
  PixelFormat.nVersion = 1;
  PixelFormat.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
  PixelFormat.cColorBits = 32;
  PixelFormat.cAlphaBits = 8;
  PixelFormat.iLayerType = PFD_MAIN_PLANE;
  PixelFormat.iPixelType = PFD_TYPE_RGBA;
  
  int PixelFormatIndex = ChoosePixelFormat(OpenGLContext.DeviceContext, &PixelFormat);
  PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
  DescribePixelFormat(OpenGLContext.DeviceContext, 
                      PixelFormatIndex, 
                      sizeof(SuggestedPixelFormat), 
                      &SuggestedPixelFormat);
  SetPixelFormat(OpenGLContext.DeviceContext, PixelFormatIndex, &SuggestedPixelFormat);

  OpenGLContext.ContextHandle = wglCreateContext(OpenGLContext.DeviceContext);
  wglMakeCurrent(OpenGLContext.DeviceContext, OpenGLContext.ContextHandle);
 
  InitializeRenderer();
#elif BCODER_RENDERER_OS == 1
  InitializeRendererWin32();

#endif

  ShowWindow(Win32Window, SW_MAXIMIZE);
  while (!IsExitRequested) {
    MSG Message;

    while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
      if (Message.message == WM_QUIT) {
        IsExitRequested = true;
      }

      TranslateMessage(&Message);
      DispatchMessageA(&Message);
    }

#if BCODER_RENDERER_OPENGL == 1
    DrawBCoderOpenGL();
    SwapBuffers(OpenGLContext.DeviceContext);
#elif BCODER_RENDERER_OS == 1
    RedrawWindow(Win32Window, NULL, NULL, RDW_INVALIDATE);
#endif

    Sleep(32);
  }


#if BCODER_RENDERER_OPENGL == 1
  wglMakeCurrent(OpenGLContext.DeviceContext, NULL);
  wglDeleteContext(OpenGLContext.ContextHandle);
  ReleaseDC(OpenGLContext.WindowHandle, OpenGLContext.DeviceContext);
#elif BCODER_RENDERER_OS == 1
  ShutdownRendererWin32();
#endif

  ShutdownBCoder();
  return 0;
}

