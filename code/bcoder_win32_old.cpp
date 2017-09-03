// bcoder project; code editor mainly for c/c++;
// author: Brajan Bartoszewicz
#include <windows.h>

#include "bcoder.h"
#include "bcoder_string.h"

// cpps
#include "bcoder.cpp"
#include "bcoder_string.cpp"

// app data
global_variable bool IsExitRequested = false;

global_variable HDC BackbufferDC;
global_variable HBITMAP BackbufferBitmap;
global_variable HFONT Font;
global_variable HWND Win32Window;

struct panel_graphics_win32 {
  HDC Context;
  HBITMAP Bitmap;
};
global_variable panel_graphics_win32 PanelsGraphicsWin32[MaxPanels];

internal void
Win32DrawRectangle(HDC DeviceContext, u32 X, u32 Y, u32 Width, u32 Height, rgb Color) {
  HBRUSH Brush = CreateSolidBrush(RGB(Color.Red, Color.Green, Color.Blue));
  RECT Rect = {
    (LONG)X, (LONG)Y,
    (LONG)X + (LONG)Width, (LONG)Y + (LONG)Height
  };
  FillRect(DeviceContext, &Rect, Brush);
  DeleteObject(Brush);
}

internal void
Win32DrawText(HDC DeviceContext, char *String, u32 Length, u32 X, u32 Y, rgb Color) {
  SelectObject(DeviceContext, Font);
  SetBkMode(DeviceContext, TRANSPARENT);
  SetTextColor(DeviceContext, RGB(Color.Red, Color.Green, Color.Blue));
  TextOutA(DeviceContext, X, Y, String, Length);  
}

internal void
Win32ResizeDIBSection(HDC DeviceContext, u32 Width, u32 Height) {
  if (!BackbufferDC) {
    BackbufferDC = CreateCompatibleDC(0);
  }

  if (BackbufferBitmap) {
    DeleteObject(BackbufferBitmap);
  }
  
  BackbufferBitmap = CreateCompatibleBitmap(DeviceContext, Width, Height);
  SelectObject(BackbufferDC, BackbufferBitmap);

  for (u32 Index = 0;
       Index < MaxPanels;
       ++Index) {
    if (!BCoderState.Panels[Index].Active)
      continue;

    if (!PanelsGraphicsWin32[Index].Context) {
      PanelsGraphicsWin32[Index].Context = CreateCompatibleDC(0);
    }

    if (PanelsGraphicsWin32[Index].Bitmap) {
      DeleteObject(PanelsGraphicsWin32[Index].Bitmap);
    }

    BCoderState.Panels[Index].DrawAreaWidth = Width * BCoderState.Panels[Index].ViewportWidth;
    BCoderState.Panels[Index].DrawAreaHeight = Height * BCoderState.Panels[Index].ViewportHeight;

    PanelsGraphicsWin32[Index].Bitmap = CreateCompatibleBitmap(DeviceContext, BCoderState.Panels[Index].DrawAreaWidth, BCoderState.Panels[Index].DrawAreaHeight);
    SelectObject(PanelsGraphicsWin32[Index].Context, PanelsGraphicsWin32[Index].Bitmap);
  }
}

struct win32_draw_lines_context {
  bool IsMultilineComment;
  bool IsSelected;
};

internal void
Win32DrawLineOfText(HDC DeviceContext, u32 ViewX, u32 PositionX, u32 PositionY, panel *Panel, char *LineStart, char *LineEnd, win32_draw_lines_context *DrawLinesCtx) {
  Assert(LineStart != 0);
  Assert(LineEnd != 0);

  SIZE Size;
  theme *Theme = &BCoderState.Theme;
  char *Buffer = LineStart;
  char *PositionInLine = LineStart;
  u32 LineLength = LineEnd - LineStart + 1;

  // NOTE(Brajan): in that order
  bool IsComment = false;
  bool IsString = false;
  bool IsReadingWord = false;
  bool IsPreprocesor = false;
  char *WordStart = 0;

  if (LineLength > 0 && Buffer[0] == '#')
    IsPreprocesor = false;

  u32 OffsetX = 0;
  for (u32 Index = 0;
       Index < LineLength;
       ++Index) {
#if 1
    if (Buffer[Index] == '/' && Buffer[Index + 1] == '*' && !IsString) {
      Win32DrawText(DeviceContext, PositionInLine, (Buffer + Index) - PositionInLine, PositionX + OffsetX, PositionY, Theme->Text);
      GetTextExtentPoint32A(DeviceContext, PositionInLine, (Buffer + Index) - PositionInLine, &Size);
      OffsetX += Size.cx;

      PositionInLine = Buffer + Index;
      Index += 2;
      DrawLinesCtx->IsMultilineComment = true;
      continue;
    } else if (DrawLinesCtx->IsMultilineComment) {
      if (Buffer[Index] == '*' && Buffer[Index + 1] == '/') {
        Index += 2;
        Win32DrawText(DeviceContext, PositionInLine, (Buffer + Index) - PositionInLine, PositionX + OffsetX, PositionY, Theme->Comment);
        GetTextExtentPoint32A(DeviceContext, PositionInLine, (Buffer + Index) - PositionInLine, &Size);
        OffsetX += Size.cx;

        DrawLinesCtx->IsMultilineComment = false;
        PositionInLine = Buffer + Index;
      }
      continue;
    }

    if (Buffer[Index] == '/' && Buffer[Index + 1] == '/') {
      Win32DrawText(DeviceContext, PositionInLine, (Buffer + Index) - PositionInLine, PositionX + OffsetX, PositionY, Theme->Text);
      GetTextExtentPoint32A(DeviceContext, PositionInLine, (Buffer + Index) - PositionInLine, &Size);
      OffsetX += Size.cx;

      Win32DrawText(DeviceContext, Buffer + Index, LineEnd - (Buffer + Index), PositionX + OffsetX, PositionY, Theme->Comment);
      IsComment = true;
      break;
    } else if (Buffer[Index] == '"') {
      if (!IsString) {
        Win32DrawText(DeviceContext, PositionInLine, (Buffer + Index) - PositionInLine, PositionX + OffsetX, PositionY, Theme->Text);
        GetTextExtentPoint32A(DeviceContext, PositionInLine, (Buffer + Index) - PositionInLine, &Size);
        OffsetX += Size.cx;
        
        PositionInLine = Buffer + Index;
        IsString = true;
      } else {
        Win32DrawText(DeviceContext, PositionInLine, (Buffer + Index) - PositionInLine + 1, PositionX + OffsetX, PositionY, Theme->String);
        GetTextExtentPoint32A(DeviceContext, PositionInLine, (Buffer + Index) - PositionInLine + 1, &Size);
        OffsetX += Size.cx;

        PositionInLine = Buffer + Index + 1;
        IsString = false;
      }
    }

    if (IsPreprocesor)
      continue;
    
    bool WasKeyword = false;
    if ((!IsDelimiter(Buffer[Index]) && !IsWhitespace(Buffer[Index])) && !IsReadingWord) {
      WordStart = Buffer + Index;
      IsReadingWord = true;
    } else if ((IsDelimiter(Buffer[Index]) || IsWhitespace(Buffer[Index])) && IsReadingWord) {
      u32 WordLength = (Buffer + Index) - WordStart;
      if (IsWordKeyword(WordStart, WordLength)) {
        Win32DrawText(DeviceContext, PositionInLine, WordStart - PositionInLine, PositionX + OffsetX, PositionY, Theme->Text);
        GetTextExtentPoint32A(DeviceContext, PositionInLine, WordStart - PositionInLine, &Size);
        OffsetX += Size.cx;

        Win32DrawText(DeviceContext, WordStart, WordLength, PositionX + OffsetX, PositionY, Theme->Keyword);
        GetTextExtentPoint32A(DeviceContext, WordStart, WordLength, &Size);
        OffsetX += Size.cx;

        PositionInLine = Buffer + Index;
      } else if (IsConstant(WordStart, WordLength)) {
        Win32DrawText(DeviceContext, PositionInLine, WordStart - PositionInLine, PositionX + OffsetX, PositionY, Theme->Text);
        GetTextExtentPoint32A(DeviceContext, PositionInLine, WordStart - PositionInLine, &Size);
        OffsetX += Size.cx;

        Win32DrawText(DeviceContext, WordStart, WordLength, PositionX + OffsetX, PositionY, Theme->Constant);
        GetTextExtentPoint32A(DeviceContext, WordStart, WordLength, &Size);
        OffsetX += Size.cx;

        PositionInLine = Buffer + Index;
      }
      IsReadingWord = false;
    }
#endif
  }

  if (DrawLinesCtx->IsMultilineComment) {
    Win32DrawText(DeviceContext, PositionInLine, LineEnd - PositionInLine, PositionX + OffsetX, PositionY, Theme->Comment);
  } else if (!IsComment) {
    Win32DrawText(DeviceContext, PositionInLine, LineEnd - PositionInLine, PositionX + OffsetX, PositionY, Theme->Text);
  }
}

internal void
Win32DrawPanel(HDC DeviceContext, panel *Panel) {
  theme *Theme = &BCoderState.Theme;
  buffer *Buffer = Panel->Buffer;
  bool IsActive = (Panel == BCoderState.CurrentPanel);

  u32 Width = Panel->DrawAreaWidth;
  u32 Height = Panel->DrawAreaHeight;
  u32 PositionX = 0;
  u32 PositionY = 0;

  win32_draw_lines_context DrawLinesCtx;
  DrawLinesCtx.IsMultilineComment = false;
  DrawLinesCtx.IsSelected = false;

  Win32DrawRectangle(DeviceContext, 0, 0, Width, Height, Theme->Background); 

  if (BCoderState.ShowNumbers) {
    char TempBuffer[16];
    wsprintf(TempBuffer, "%d", GetLinesNumber(Buffer));

    SIZE Size;
    GetTextExtentPoint32A(DeviceContext, TempBuffer, StringLength(TempBuffer), &Size);
    PositionX = Size.cx + 5;

    Win32DrawRectangle(DeviceContext, 0, 0, PositionX, Height, Theme->StatusBarBackground);
  }

  char StatusBarString[512];
  bool Saved = Buffer->Flags & BufferFlag_Saved;
  if (Buffer->Flags & BufferFlag_NotSavable) {
    Saved = true;
  }
  wsprintf(StatusBarString, "File: %s%c L:%d C:%d LL:%d [%s] DEBUG: CHAR: %c ASCII: %d", 
           Buffer->Name,
           (Saved ? ' ' : '*'),
           GetLineNumberByPosition(Buffer, Panel->CursorPosition),
           CalculateCursorPositionInLine(Buffer, Panel->CursorPosition),
           GetLineLength(Buffer, GetLineNumberByPosition(Buffer, Panel->CursorPosition)),
           (Buffer->LineEnding == LineEnding_Dos ? "dos" : "unix"),
           *((char *)Buffer->Memory + Panel->CursorPosition),
           *((char *)Buffer->Memory + Panel->CursorPosition));
  if (IsActive) {
    Win32DrawRectangle(DeviceContext, 0, 0, Width, BCoderState.LineHeight, Theme->StatusBarBackground);
    Win32DrawText(DeviceContext, StatusBarString, lstrlenA(StatusBarString), 4, 0, Theme->StatusBarText);
  } else {
    Win32DrawRectangle(DeviceContext, 0, 0, Width, BCoderState.LineHeight, Theme->InactiveStatusBarBackground);
    Win32DrawText(DeviceContext, StatusBarString, lstrlenA(StatusBarString), 4, 0, Theme->InactiveStatusBarText);
  }
  
  u32 MarginTop = BCoderState.LineHeight + 4;

  u32 CursorScreenPosX = 0;
  u32 CursorScreenPosY = 0;
  u32 LineIndex = 0;
  char *StartPosition = (char *)Buffer->Memory;
  char *CurrentPosition = StartPosition + GetPositionWhereLineStarts(Buffer, Panel->ViewLine);

  u32 SelectionRectangleStartX = 0;

  u32 SelectStart = 0;
  u32 SelectEnd = 0;
  bool IsSelecting = false;

  if (Panel->SelectStart > Panel->CursorPosition) {
    SelectStart = Panel->CursorPosition;
    SelectEnd = Panel->SelectStart;
  } else {
    SelectStart = Panel->SelectStart;
    SelectEnd = Panel->CursorPosition;
  }

  u32 LineLength = 0;
  u32 SelectionDrawStart = 0;
  u32 LineStartGlobPosition = 0;
  bool SelectionStartsHere = false;

  if (Panel->ViewLine > GetLineNumberByPosition(Panel->Buffer, Panel->SelectStart)) {
    SelectionStartsHere = false;
    IsSelecting = true;
    SelectionDrawStart = 0;
  }

  for (;;) {
    if (Buffer->Length == 0)
      break;

    if (*CurrentPosition == '\0')
      break;

    if (LineIndex > GetMaxLinesOnPanel(Height, BCoderState.LineHeight))
      break;

    char *LineStart = CurrentPosition;
    for (;;) {
      if (StartPosition + Panel->CursorPosition == CurrentPosition) {
        SIZE Size;
        GetTextExtentPoint32A(DeviceContext, LineStart, CurrentPosition - LineStart, &Size);
        CursorScreenPosX = Size.cx;
      }

      if (*CurrentPosition == '\0' || *CurrentPosition == '\n') {
        break;
      }
      ++CurrentPosition;
    }

    if (BCoderState.ShowNumbers) {
      char TempBuffer[16] = "0";
      wsprintf(TempBuffer, "%d", LineIndex + Panel->ViewLine + 1);
      Win32DrawText(DeviceContext, TempBuffer, lstrlenA(TempBuffer), 0, MarginTop + (LineIndex * BCoderState.LineHeight), Theme->InactiveStatusBarText);
    }

    if (Panel->Selecting) {
      PositionY = MarginTop + (LineIndex * BCoderState.LineHeight);
      LineLength = CurrentPosition - LineStart + 1;
      SelectionDrawStart = 0;
      LineStartGlobPosition = (u32)(LineStart - StartPosition);
      SelectionStartsHere = false;

      for (u32 Index = 0;
           Index < LineLength;
           ++Index) {
        SIZE Size;
        u32 GlobPosition = (u32)(LineStart + Index - StartPosition);
        if (GlobPosition == SelectStart) {
          GetTextExtentPoint32A(DeviceContext, LineStart, GlobPosition - LineStartGlobPosition, &Size);
          SelectionDrawStart = Size.cx;
          IsSelecting = true;
          SelectionStartsHere = true;
        }
        
        if (GlobPosition == SelectEnd) {
          char *SelectionStartPtr = StartPosition + SelectStart;
          char *SelectionEndPtr = LineStart + Index;
          
          if (LineStart > SelectionStartPtr)
            SelectionStartPtr = LineStart;

          GetTextExtentPoint32A(DeviceContext, SelectionStartPtr, SelectionEndPtr - SelectionStartPtr, &Size);
          u32 SelectionDrawWidth = Size.cx;
          Win32DrawRectangle(DeviceContext, SelectionDrawStart, PositionY, SelectionDrawWidth, BCoderState.LineHeight, Theme->Selection);
          IsSelecting = false;
          SelectionStartsHere = false;
        }

        if (Index == LineLength - 1 && IsSelecting) {
          if (SelectionStartsHere) {
            char *SelectionStartPtr = StartPosition + SelectStart;
            GetTextExtentPoint32A(DeviceContext, SelectionStartPtr, CurrentPosition - SelectionStartPtr, &Size);
          } else {
            GetTextExtentPoint32A(DeviceContext, LineStart, CurrentPosition - LineStart, &Size);
          }
          u32 SelectionDrawWidth = Size.cx;
          if (SelectionDrawWidth == 0)
            SelectionDrawWidth = 2;
          Win32DrawRectangle(DeviceContext, SelectionDrawStart, PositionY, SelectionDrawWidth, BCoderState.LineHeight, Theme->Selection);
        }
      }
    }

    Win32DrawLineOfText(DeviceContext, 0, PositionX, MarginTop + (LineIndex * BCoderState.LineHeight), Panel, LineStart, CurrentPosition, &DrawLinesCtx);

    ++LineIndex;
    ++CurrentPosition;
  }

  // cursor
  if (!IsActive)
    return;
  if (!Panel->ShowCommandPrompt) {
    u32 LineByPosition = GetLineNumberByPosition(Buffer, Panel->CursorPosition);
    u32 LineOnScreen = LineByPosition - Panel->ViewLine;
    CursorScreenPosY = MarginTop + (LineOnScreen * BCoderState.LineHeight);
    Win32DrawRectangle(DeviceContext, CursorScreenPosX + PositionX, CursorScreenPosY, 2, BCoderState.LineHeight + 3, Theme->Cursor);
  } else if (Panel->ShowCommandPrompt) {
    Win32DrawRectangle(DeviceContext, 0, BCoderState.LineHeight + 1, Panel->DrawAreaWidth, BCoderState.LineHeight, Theme->StatusBarBackground);
    
    SIZE Size;
    Win32DrawText(DeviceContext, (char *)Panel->CommandBuffer.Memory, Panel->CommandBuffer.Length, 4, MarginTop + 1, Theme->StatusBarText);
    u32 CursorX = 0;
    
    char *Start = (char *)Panel->CommandBuffer.Memory;
    char *CurrentPosition = Start;
    for (;;) {
      if (Start + Panel->CommandCursorPosition == CurrentPosition) {
        GetTextExtentPoint32A(DeviceContext, Start, CurrentPosition - Start, &Size);
        CursorX += Size.cx;
        break;
      }

      if (*CurrentPosition == '\0' || *CurrentPosition == '\n') {
        break;
      }
      ++CurrentPosition;
    }

    Win32DrawRectangle(DeviceContext, CursorX + 4, BCoderState.LineHeight, 2, MarginTop + 1, Theme->StatusBarText);
  }
}

internal void
Win32UpdateWindow(HDC DeviceContext, u32 X, u32 Y, u32 Width, u32 Height) {
  for (u32 Index = 0;
       Index < ArrayCount(BCoderState.Panels);
       ++Index) {
    panel *Panel = &BCoderState.Panels[Index];

    if (!Panel->Active)
      continue;

    Win32DrawPanel(PanelsGraphicsWin32[Index].Context, Panel);
    BitBlt(DeviceContext, Panel->ViewportX * Width, Panel->ViewportY * Height, 
           Panel->DrawAreaWidth, Panel->DrawAreaHeight,
           PanelsGraphicsWin32[Index].Context, 0, 0, SRCCOPY);
  }
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

internal void
Win32RecreatePanelsGraphics() {
  SendMessage(Win32Window, WM_SIZE, 0, 0);
}

internal void
Win32LoadSystemFont(u32 SizeInPixels, const char *FontName) {
  if (Font) {
    DeleteObject(Font);
  }
  Font = CreateFont(SizeInPixels, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FontName);
  BCoderState.LineHeight = SizeInPixels;
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

internal void
Win32Redraw() {
  RedrawWindow(Win32Window, NULL, NULL, RDW_INVALIDATE);
}

internal LRESULT CALLBACK
WindowCallback(HWND Handle, UINT Message, WPARAM WParam, LPARAM LParam) {
  LRESULT Result = 0;

  switch (Message) {
    case WM_QUIT: {
      IsExitRequested = true;
    } break;

    case WM_DESTROY: {
      IsExitRequested = true;
    } break;

    case WM_CLOSE: {
      IsExitRequested = true;
    } break;
    
    case WM_SIZE: {
      HDC DeviceContext = GetDC(Handle);
      RECT Rect;
      GetClientRect(Handle, &Rect);
      u32 Width = Rect.right - Rect.left;
      u32 Height = Rect.bottom - Rect.top;
      Win32ResizeDIBSection(DeviceContext, Width, Height);
      ReleaseDC(Handle, DeviceContext);
    } break;

    case WM_SYSKEYDOWN:
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

    case WM_SYSKEYUP:
    case WM_KEYUP: {
    } break;

    case WM_PAINT: {
      PAINTSTRUCT PaintStruct;
      HDC DeviceContext;
      DeviceContext = BeginPaint(Handle, &PaintStruct);
      u32 X = PaintStruct.rcPaint.left;
      u32 Y = PaintStruct.rcPaint.top;
      u32 Width = PaintStruct.rcPaint.right - PaintStruct.rcPaint.left;
      u32 Height = PaintStruct.rcPaint.bottom - PaintStruct.rcPaint.top;
      Win32UpdateWindow(BackbufferDC, 0, 0, Width, Height);
      BitBlt(DeviceContext, 0, 0, Width, Height, BackbufferDC, 0, 0, SRCCOPY);
      EndPaint(Handle, &PaintStruct); 
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
  Platform.ReadFileIntoBuffer = Win32ReadFileIntoBuffer;
  Platform.WriteBufferIntoFile = Win32WriteBufferIntoFile;
  Platform.ShowError = Win32ShowError;
  Platform.CreateEmptyBuffer = Win32CreateEmptyBuffer;
  Platform.FreeBuffer = Win32FreeBuffer;
  Platform.GetSystemClipboard = Win32GetSystemClipboard;
  Platform.SetSystemClipboard = Win32SetSystemClipboard;
  Platform.AllocateMemory = Win32AllocateMemory;
  Platform.FreeMemory = Win32FreeMemory;
  Platform.RecreatePanelsGraphics = Win32RecreatePanelsGraphics;
  Platform.LoadSystemFont = Win32LoadSystemFont;
  Platform.GetAsyncKeysState = Win32GetAsyncKeysState;
  Platform.RunBuildFile = Win32RunBuildFile;
  Platform.Redraw = Win32Redraw;

  // initialize bcoder before creating win32 window
  InitializeBCoder();

  // create win32 window
  WNDCLASS WindowClass = {};
  WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  WindowClass.cbClsExtra = 0;
  WindowClass.cbWndExtra = 0;
  WindowClass.lpszClassName = "bcoder_win32";
  WindowClass.hInstance = Instance;
  WindowClass.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
  WindowClass.lpszMenuName = NULL;
  WindowClass.lpfnWndProc = &WindowCallback;
  WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  WindowClass.hIcon = NULL;

  RegisterClass(&WindowClass);

  DWORD Flags = WS_VISIBLE;
  Flags |= WS_CAPTION | WS_MINIMIZEBOX;
  Flags |= WS_THICKFRAME | WS_MAXIMIZEBOX;
  Flags |= WS_SYSMENU;

  Win32Window = 
    CreateWindowA("bcoder_win32", "bcoder",
                  Flags,
                  0, 0, 640, 480,
                  NULL, NULL, Instance, NULL);

  ShowWindow(Win32Window, SW_MAXIMIZE);
  while (!IsExitRequested) {
    MSG Message;
    while (PeekMessageW(&Message, NULL, 0, 0, PM_NOREMOVE)) {
      if (!GetMessage(&Message, NULL, 0, 0)) {
        break;
      }

      DispatchMessage(&Message);
      TranslateMessage(&Message);
    }

    DrawAndUpdate();

    Sleep(1000 / 60);
  }
  
  if (Font) {
    DeleteObject(Font);
  }

  ShutdownBCoder();
  return 0;
}

