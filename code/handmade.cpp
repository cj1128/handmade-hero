#include "handmade.h"

internal void
RenderWeirdGradeint(game_offscreen_buffer *Buffer, int XOffset, int YOffset) {
  uint8 *Row = (uint8 *)Buffer->Memory;

  for(int Y = 0; Y < Buffer->Height; Y++) {
    uint32 *Pixel = (uint32 *)Row;

    for(int X = 0; X < Buffer->Width; X++) {
      uint8 Blue = X + XOffset;
      uint8 Green = Y + YOffset;
      // 0xXXRRGGBB
      *Pixel++ = (Green << 8) | Blue;
    }

    Row = Row + Buffer->Width * Buffer->BytesPerPixel;
  }
}

internal void
GameUpdateAndRender(game_offscreen_buffer *Buffer, int XOffset, int YOffset) {
  RenderWeirdGradeint(Buffer, XOffset, YOffset);
}
