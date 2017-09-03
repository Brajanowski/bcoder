#ifndef BCODER_RENDERER_OPENGL_H_

#include "bcoder.h"

struct float_rgb {
  float Red, Green, Blue;
};

struct float_rgba {
  float Red, Green, Blue, Alpha;
};

void InitializeRenderer();
void DrawBCoderOpenGL();
void DrawPanel(panel *Panel);
void UpdatePanels();

float_rgb RGBToFloatRGB(rgb Color);
float_rgba RGBToFloatRGBA(rgb Color);
float_rgba RGBAToFloatRGBA(rgba Color);

void DrawTexturedRectangle(float PositionX, float PositionY, float Width, float Height, u32 Texture);
void DrawRectangle(float PositionX, float PositionY, float Width, float Height, float_rgba Color);
void DrawCharacter(char Character, float PositionX, float PositionY, float_rgba Color);
void DrawString(char *String, u32 StringLength, float PositionX, float PositionY, float_rgba Color);

#define BCODER_RENDERER_OPENGL_H_
#endif

