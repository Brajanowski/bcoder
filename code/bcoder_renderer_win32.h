#ifndef BCODER_RENDERER_WIN32_

#include "bcoder.h"

struct panel_bitmap_win32 {
  HDC Context;
  HBITMAP Bitmap;
};

struct renderer_win32 {
  HDC BackbufferDC;
  HBITMAP BackbufferBitmap;
  HFONT Font;

  panel_bitmap_win32 PanelsBitmapsWin32[MaxPanels];
};

global_variable renderer_win32 Renderer;

void InitializeRendererWin32();
void ShutdownRendererWin32();
void DrawBCoderWin32();
void RecreatePanelsBitmapsWin32();
void WindowResizedWin32(HDC DeviceContext);
void SetFontWin32(int FontSize, const char *Filename);

void DrawPanel(HDC DeviceContext, panel *Panel);

void DrawRectangle(HDC DeviceContext, u32 X, u32 Y, u32 Width, u32 Height, rgb Color);
void DrawText(HDC DeviceContext, char *String, u32 Length, int X, int Y, rgb Color);

#define BCODER_RENDERER_WIN32_
#endif
