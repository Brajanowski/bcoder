#include "bcoder_renderer_opengl.h"
#include "bcoder_string.h"

#include <GL/gl.h>

global_variable u32 FontAtlasTexture = 0;

void
InitializeRenderer() {
  glEnable(GL_TEXTURE_2D);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glDisable(GL_DEPTH_TEST);

  glGenTextures(1, &FontAtlasTexture);
  glBindTexture(GL_TEXTURE_2D, FontAtlasTexture);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, BCoderState.Font.Atlas.Width, BCoderState.Font.Atlas.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (u8 *)BCoderState.Font.Atlas.Memory);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void
DrawBCoderOpenGL() {
  float_rgb BackgroundColor = RGBToFloatRGB(BCoderState.Theme.Background);

  glClearColor(BackgroundColor.Red, BackgroundColor.Green, BackgroundColor.Blue, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  glViewport(0, 0, BCoderState.WindowWidth, BCoderState.WindowHeight);

  // set up orthographic projection to match screen coordinates
  glLoadIdentity();
  glOrtho(0, BCoderState.WindowWidth, BCoderState.WindowHeight, 0, 0, 100);

  //DrawTexturedRectangle(0, 0, 1024, 1024, FontAtlasTexture);
  
  for (u32 Index = 0;
       Index < MaxPanels;
       ++Index) {
    if (!BCoderState.Panels[Index].Active)
      continue;
    
    panel *Panel = &BCoderState.Panels[Index];
    DrawPanel(Panel);
  }
}

void
DrawPanel(panel *Panel) {
  theme *Theme = &BCoderState.Theme;
  buffer *Buffer = Panel->Buffer;
  bool IsActive = (Panel == BCoderState.CurrentPanel);
  int FontSize = BCoderState.Font.Size;
  
  DrawRectangle(0, 0, Panel->DrawAreaWidth, FontSize, RGBToFloatRGBA(Theme->StatusBarBackground));

  float PositionX = 0;
  float PositionY = FontSize;
  char *Character = (char *)Buffer->Memory;
  
  while (*Character != '\0') {
    if ((u32)(Character - (char *)Buffer->Memory) == Panel->CursorPosition) {
      DrawRectangle(PositionX, PositionY, 2, FontSize, RGBToFloatRGBA(Theme->Cursor));
    }

    if (*Character == '\n' || *Character == '\r') {
      PositionX = 0;
      PositionY += FontSize;
      ++Character;

      if (*Character == '\n') {
        ++Character;
      }

      continue;
    }

    Assert(*Character - StartCharacter < GlyphsNumber);
    glyph *Glyph = &BCoderState.Font.Glyphs[*Character - StartCharacter];

    DrawCharacter(*Character, PositionX + Glyph->XOffset, PositionY + Glyph->YOffset, RGBToFloatRGBA(Theme->Text));
    PositionX += Glyph->XAdvance;

    if (PositionY > Panel->DrawAreaHeight - FontSize) {
      break;
    }

    ++Character;
  }

  char StatusBarString[512] = "File: w:/test.c L:0 C:0 LL:0";
  DrawString(StatusBarString, StringLength(StatusBarString), 0, 0, RGBToFloatRGBA(Theme->StatusBarText));
}

void
UpdatePanels() {
  for (u32 Index = 0;
       Index < MaxPanels;
       ++Index) {
    if (!BCoderState.Panels[Index].Active)
      continue;

    DestroyBitmap(&BCoderState.Panels[Index].Bitmap);

    BCoderState.Panels[Index].DrawAreaWidth = BCoderState.WindowWidth * BCoderState.Panels[Index].ViewportWidth;
    BCoderState.Panels[Index].DrawAreaHeight = BCoderState.WindowHeight * BCoderState.Panels[Index].ViewportHeight;
    
    //BCoderState.Panels[Index].Bitmap = CreateBitmap(BCoderState.Panels[Index].DrawAreaWidth, BCoderState.Panels[Index].DrawAreaHeight, 4);
  }
}

float_rgb
RGBToFloatRGB(rgb Color) {
  float_rgb FloatColor = {};
  FloatColor.Red = (float)Color.Red / 255.0f;
  FloatColor.Green = (float)Color.Green / 255.0f;
  FloatColor.Blue = (float)Color.Blue / 255.0f;
  return FloatColor;
}

float_rgba
RGBToFloatRGBA(rgb Color) {
  float_rgba FloatColor = {};
  FloatColor.Red = (float)Color.Red / 255.0f;
  FloatColor.Green = (float)Color.Green / 255.0f;
  FloatColor.Blue = (float)Color.Blue / 255.0f;
  FloatColor.Alpha = 1.0f;
  return FloatColor;
}

float_rgba
RGBAToFloatRGBA(rgba Color) {
  float_rgba FloatColor = {};
  FloatColor.Red = (float)Color.Red / 255.0f;
  FloatColor.Green = (float)Color.Green / 255.0f;
  FloatColor.Blue = (float)Color.Blue / 255.0f;
  FloatColor.Alpha = (float)Color.Alpha / 255.0f;
  return FloatColor;
}

void
DrawRectangle(float PositionX, float PositionY, float Width, float Height, float_rgba Color) {
  glBindTexture(GL_TEXTURE_2D, 0);
  glColor4f(Color.Red, Color.Green, Color.Blue, Color.Alpha);
  glBegin(GL_QUADS);
    glVertex2f(PositionX, PositionY);
    glVertex2f(PositionX + Width, PositionY);
    glVertex2f(PositionX + Width, PositionY + Height);
    glVertex2f(PositionX, PositionY + Height);
  glEnd();
}

void
DrawTexturedRectangle(float PositionX, float PositionY, float Width, float Height, u32 Texture) {
  glBindTexture(GL_TEXTURE_2D, Texture);

  glColor4f(1, 1, 1, 1);
  glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(PositionX, PositionY);

    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(PositionX + Width, PositionY);

    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(PositionX + Width, PositionY + Height);

    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(PositionX, PositionY + Height);
  glEnd();
}

void
DrawCharacter(char Character, float PositionX, float PositionY, float_rgba Color) {
  glBindTexture(GL_TEXTURE_2D, FontAtlasTexture);
  glyph *Glyph = &BCoderState.Font.Glyphs[Character - StartCharacter];

  float FontAtlasWidth = BCoderState.Font.Atlas.Width;
  float FontAtlasHeight = BCoderState.Font.Atlas.Height;

  float StartU = ((float)Glyph->X0) / FontAtlasWidth;
  float StartV = ((float)Glyph->Y0) / FontAtlasHeight;

  float EndU = ((float)Glyph->X1) / FontAtlasWidth;
  float EndV = ((float)Glyph->Y1) / FontAtlasHeight;

  glColor4f(Color.Red, Color.Green, Color.Blue, Color.Alpha);
  glBegin(GL_QUADS);
    glTexCoord2f(StartU, StartV);
    glVertex2f(PositionX, PositionY);

    glTexCoord2f(EndU, StartV);
    glVertex2f(PositionX + Glyph->Width, PositionY);

    glTexCoord2f(EndU, EndV);
    glVertex2f(PositionX + Glyph->Width, PositionY + Glyph->Height);

    glTexCoord2f(StartU, EndV);
    glVertex2f(PositionX, PositionY + Glyph->Height);
  glEnd();
}

// TODO(Brajan): maybe I can optimize this?
void
DrawString(char *String, u32 StringLength, float PositionX, float PositionY, float_rgba Color) {
  Assert(String != 0);

  float CurrentPositionX = 0;
  for (u32 Index = 0; Index < StringLength; ++Index) {
    Assert(String[Index] - StartCharacter < GlyphsNumber);
    
    glyph *Glyph = &BCoderState.Font.Glyphs[String[Index] - StartCharacter];

    DrawCharacter(String[Index], PositionX + CurrentPositionX + Glyph->XOffset, PositionY + Glyph->YOffset, Color);
    CurrentPositionX += Glyph->XAdvance;
  }
}

#if 0
void
DrawCharacter(bitmap* Destination, char Character, int PosX, int PosY) {
  glyph *Glyph = &BCoderState.Font.Glyphs[Character - StartCharacter];
  BitmapBlit(&BCoderState.Font.Atlas, Destination, PosX, PosY, Glyph->X0, Glyph->Y0, Glyph->Width, Glyph->Height);
}

void
DrawString(bitmap *Destination, char *String, u32 StringLength, int PosX, int PosY) {
  Assert(Destination != 0);
  Assert(String != 0);

  int CurrentPositionX = 0;
  for (u32 Index = 0; Index < StringLength; ++Index) {
    Assert(String[Index] - StartCharacter < GlyphsNumber);
    
    glyph *Glyph = &BCoderState.Font.Glyphs[String[Index] - StartCharacter];

    DrawCharacter(Destination, String[Index], PosX + CurrentPositionX + Glyph->XOffset, PosY + Glyph->YOffset);
    //DrawCharacter(Destination, String[Index], PosX + CurrentPositionX + Glyph->XOffset, PosY + Glyph->YOffset + BCoderState.Font.Size);
    CurrentPositionX += Glyph->XAdvance;
  }
}
#endif
