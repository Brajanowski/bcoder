#include "bcoder_renderer_win32.h"
#include "bcoder_string.h"

#include "bcoder_lexer.h"

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

internal lexer_context LexerContext;

void
InitializeRendererWin32() {
  Renderer.BackbufferDC = CreateCompatibleDC(0);

  LexerInit(&LexerContext);
}

void
ShutdownRendererWin32() {
  LexerShutdown(&LexerContext);
}

void
DrawBCoderWin32() {
  for (u32 Index = 0;
       Index < MaxPanels;
       ++Index) {
    panel *Panel = &BCoderState.Panels[Index];
    if (!Panel->Active) {
      continue;
    }

    DrawPanel(Renderer.PanelsBitmapsWin32[Index].Context, Panel);
    BitBlt(Renderer.BackbufferDC, 
           Panel->ViewportX * BCoderState.WindowWidth, Panel->ViewportY * BCoderState.WindowHeight, 
           Panel->DrawAreaWidth, Panel->DrawAreaHeight,
           Renderer.PanelsBitmapsWin32[Index].Context, 0, 0, SRCCOPY);
  }
}

void
RecreatePanelsBitmapsWin32() {
  for (u32 Index = 0;
       Index < MaxPanels;
       ++Index) {
    if (Renderer.PanelsBitmapsWin32[Index].Bitmap) {
      DeleteObject(Renderer.PanelsBitmapsWin32[Index].Bitmap);
    }

    if (!BCoderState.Panels[Index].Active) {

      if (Renderer.PanelsBitmapsWin32[Index].Context) {
        DeleteDC(Renderer.PanelsBitmapsWin32[Index].Context);
      }

      continue;
    }

    if (!Renderer.PanelsBitmapsWin32[Index].Context) {
      Renderer.PanelsBitmapsWin32[Index].Context = CreateCompatibleDC(0);
    }

    BCoderState.Panels[Index].DrawAreaWidth = BCoderState.WindowWidth * BCoderState.Panels[Index].ViewportWidth;
    BCoderState.Panels[Index].DrawAreaHeight = BCoderState.WindowHeight * BCoderState.Panels[Index].ViewportHeight;

    Renderer.PanelsBitmapsWin32[Index].Bitmap = CreateCompatibleBitmap(Renderer.BackbufferDC, BCoderState.Panels[Index].DrawAreaWidth, BCoderState.Panels[Index].DrawAreaHeight);
    SelectObject(Renderer.PanelsBitmapsWin32[Index].Context, Renderer.PanelsBitmapsWin32[Index].Bitmap);
  }
}

void
WindowResizedWin32(HDC DeviceContext) {
  if (Renderer.BackbufferBitmap) {
    DeleteObject(Renderer.BackbufferBitmap);
  }
  
  Renderer.BackbufferBitmap = CreateCompatibleBitmap(DeviceContext,
                                                     BCoderState.WindowWidth,
                                                     BCoderState.WindowHeight);
  SelectObject(Renderer.BackbufferDC, Renderer.BackbufferBitmap);
}

void
SetFontWin32(int FontSize, const char *Filename) {
  if (Renderer.Font) {
    DeleteObject(Renderer.Font);
  }
  AddFontResourceExA(Filename, FR_PRIVATE, 0);
    
  Renderer.Font = CreateFontA(FontSize, 0, 0, 0,
                              FW_NORMAL,
                              0, 0, 0, 
                              DEFAULT_CHARSET, 
                              OUT_DEFAULT_PRECIS,
                              CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY,
                              DEFAULT_PITCH | FF_DONTCARE,
                              "Liberation Mono");
  BCoderState.LineHeight = FontSize;
}

void
DrawPanel(HDC DeviceContext, panel *Panel) {
  theme *Theme = &BCoderState.Theme;
  bool IsActive = (Panel == BCoderState.CurrentPanel);
  buffer *Buffer = Panel->Buffer;

  u32 Width = Panel->DrawAreaWidth;
  u32 Height = Panel->DrawAreaHeight;

  // background
  DrawRectangle(DeviceContext, 0, 0,
                Panel->DrawAreaWidth, Panel->DrawAreaHeight,
                Theme->Background);

  // status bar
  char StatusBarString[256];
  bool Saved = Buffer->Flags & BufferFlag_Saved;
  if (Buffer->Flags & BufferFlag_NotSavable) {
    Saved = true;
  }

  stbsp_sprintf(StatusBarString, "File: %s%c L:%d C:%d LL:%d [%s] DEBUG: CHAR: %c ASCII: %d", 
                Buffer->Name,
                (Saved ? ' ' : '*'),
                GetLineNumberByPosition(Buffer, Panel->CursorPosition),
                CalculateCursorPositionInLine(Buffer, Panel->CursorPosition),
                GetLineLength(Buffer, GetLineNumberByPosition(Buffer, Panel->CursorPosition)),
                (Buffer->LineEnding == LineEnding_Dos ? "dos" : "unix"),
                *((char *)Buffer->Memory + Panel->CursorPosition),
                *((char *)Buffer->Memory + Panel->CursorPosition));
  if (IsActive) {
    DrawRectangle(DeviceContext, 0, 0, Width, BCoderState.LineHeight, Theme->StatusBarBackground);
    DrawText(DeviceContext, StatusBarString, StringLength(StatusBarString), 4, 0, Theme->StatusBarText);
  } else {
    DrawRectangle(DeviceContext, 0, 0, Width, BCoderState.LineHeight, Theme->InactiveStatusBarBackground);
    DrawText(DeviceContext, StatusBarString, StringLength(StatusBarString), 4, 0, Theme->InactiveStatusBarText);
  }

  Tokenize(&LexerContext, (char *)Buffer->Memory, Buffer->Length);
  token *Token = LexerContext.Tokens;
  int TokensNumber = LexerContext.TokensNumber;

  int PositionX = 0;
  int PositionY = BCoderState.LineHeight;

  int SpaceWidth = 0;
  {
    SIZE Size;
    GetTextExtentPoint32A(DeviceContext, " ", 1, &Size);
    SpaceWidth = Size.cx;
  }

  while (TokensNumber--) {
    if (Token->Type == Token_EOF) {
      PositionX = 0;
      PositionY += BCoderState.LineHeight;
    } else if (Token->Type == Token_Whitespace) {
      PositionX += SpaceWidth * Token->Length;
    } else {
      rgb Color = Theme->Text;

      if (Token->Type == Token_String) {
        Color = Theme->String;
      } else if (Token->Type == Token_Comment) {
        Color = Theme->Comment;
      } else if (Token->Type == Token_Preprocesor) {
        Color = Theme->Preprocesor;
      }  else if (Token->Type == Token_Operator) {
        Color = Theme->Operator;
      }

      DrawText(DeviceContext, Token->Value, Token->Length, PositionX, PositionY, Color);
      
      SIZE Size;
      GetTextExtentPoint32A(DeviceContext, Token->Value, Token->Length, &Size);
      PositionX += Size.cx;
    }

    Token++;
  }

  /*char *Character = (char *)Buffer->Memory + (GetPositionWhereLineStarts(Panel->Buffer, Panel->ViewLine));
  while (*Character != '\0') {
    if (PositionY > Panel->DrawAreaHeight - BCoderState.LineHeight) {
      break;
    }

    if (IsActive && (u32)(Character - (char *)Buffer->Memory) == Panel->CursorPosition) {
      DrawRectangle(DeviceContext, PositionX, PositionY, 2, BCoderState.LineHeight, Theme->Cursor);
    }

    if (*Character == '\n') {
      PositionX = 0;
      PositionY += BCoderState.LineHeight;
      ++Character;
      continue;
    } else if (*Character == '\r') {
      ++Character;
      continue;
    }

    DrawText(DeviceContext, Character, 1, PositionX, PositionY, Theme->Text);

    SIZE Size;
    GetTextExtentPoint32A(DeviceContext, Character, 1, &Size);
    PositionX += Size.cx;

    ++Character;
  }*/
}

void
DrawRectangle(HDC DeviceContext, u32 X, u32 Y, u32 Width, u32 Height, rgb Color) {
  HBRUSH Brush = CreateSolidBrush(RGB(Color.Red, Color.Green, Color.Blue));
  RECT Rect = {
    (LONG)X, (LONG)Y,
    (LONG)X + (LONG)Width, (LONG)Y + (LONG)Height
  };
  FillRect(DeviceContext, &Rect, Brush);
  DeleteObject(Brush);
}

void
DrawText(HDC DeviceContext, char *String, u32 Length, int X, int Y, rgb Color) {
  SelectObject(DeviceContext, Renderer.Font);
  SetBkMode(DeviceContext, TRANSPARENT);
  SetTextColor(DeviceContext, RGB(Color.Red, Color.Green, Color.Blue));
  TextOutA(DeviceContext, X, Y, String, Length);  
}

