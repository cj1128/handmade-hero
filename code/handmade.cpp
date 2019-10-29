#include "handmade.h"

internal void
RenderPlayer(game_offscreen_buffer *Buffer, int PlayerX, int PlayerY) {
  uint8 *Start = (uint8 *)Buffer->Memory;
  uint8 *EndOfBuffer = Start + Buffer->Pitch * Buffer->Height;

  // 10 x 10 square
  if(PlayerX >= 0 && PlayerX < Buffer->Width - 10 && PlayerY >= 0 && PlayerY < Buffer->Height - 10) {

    uint8 *Row = Start + PlayerY * Buffer->Pitch + PlayerX * Buffer->BytesPerPixel;

    for(int Y = 0; Y < 10; Y++) {
      uint8 *Pixel = Row;

      for(int X = 0; X < 10; X++) {
        Assert(Pixel >= Buffer->Memory && Pixel < EndOfBuffer);
        *((uint32*)Pixel) = 0xFFFFFFFF;
        Pixel += Buffer->BytesPerPixel;
      }

      Row += Buffer->Pitch;
    }
  }
}

internal void
RenderWeirdGradeint(game_offscreen_buffer *Buffer, int XOffset, int YOffset) {
  uint8 *Row = (uint8 *)Buffer->Memory;

  for(int Y = 0; Y < Buffer->Height; Y++) {
    uint32 *Pixel = (uint32 *)Row;

    for(int X = 0; X < Buffer->Width; X++) {
      uint8 Blue = (uint8)(X + XOffset);
      uint8 Green = (uint8)(Y + YOffset);
      // 0xXXRRGGBB
      *Pixel++ = (Green << 16) | Blue;
    }

    Row = Row + Buffer->Pitch;
  }
}

internal void
RenderSineWave(
  game_state *State,
  game_sound_buffer *Buffer
) {
  int ToneHz = State->ToneHz;
  int WavePeriod = Buffer->SamplesPerSecond / ToneHz;

  int16* Output = Buffer->Memory;
  for(int SampleIndex = 0; SampleIndex < Buffer->SampleCount; SampleIndex++) {
    int16 Value = (int16)(sinf(State->tSine) * Buffer->ToneVolume);
    *Output++ = Value;
    *Output++ = Value;
    State->tSine += (real32)(2.0f * Pi32 * (1.0f / (real32)WavePeriod));
    if(State->tSine > 2.0f * Pi32) {
      State->tSine -= (real32)(2.0f * Pi32);
    }
  }
}

extern "C" GAME_UPDATE_VIDEO(GameUpdateVideo) {
  Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == ArrayCount(Input->Controllers[0].Buttons))

  game_state *State = (game_state *)Memory->PermanentStorage;

  Assert(sizeof(game_state) <= Memory->PermanentStorageSize)
  if(!Memory->IsInitialized) {
    char *Filename = __FILE__;

    debug_read_file_result File = Memory->DebugPlatformReadFile(Thread, Filename);
    if(File.Memory)
    {
        Memory->DebugPlatformWriteFile(Thread, "w:\\build\\test.out", File.Memory, File.Size);
        Memory->DebugPlatformFreeFileMemory(Thread, File.Memory);
    }

    State->ToneHz = 256;
    State->tSine = 0;
    State->PlayerX = 10;
    State->PlayerY = 10;
    Memory->IsInitialized = true;
  }

  int delta = 5;
  game_controller_input *Controller = &Input->Controllers[0];
  if(Controller->MoveUp.IsEndedDown) {
    State->PlayerY -= delta;
    // State->YOffset += delta;
  }
  if(Controller->MoveDown.IsEndedDown) {
    State->PlayerY += delta;
    // State->YOffset -= delta;
  }
  if(Controller->MoveLeft.IsEndedDown) {
    State->PlayerX -= delta;
    // State->XOffset -= delta;
  }
  if(Controller->MoveRight.IsEndedDown) {
    State->PlayerX += delta;
    // State->XOffset += delta;
  }

  if(Controller->ActionUp.IsEndedDown) {
    State->tJump = (real32)(2.0f * Pi32);
  }
  if(State->tJump > 0) {
    State->PlayerX += 5;
    State->PlayerY += (int)(sinf(State->tJump) * 20);
    State->tJump -= 0.2f;
  }

  if(State->YOffset <= 1000 && State->YOffset >= -1000) {
    int Tmp = (int)(((real32)State->YOffset / 1000) * 256) + 512;
    State->ToneHz = Tmp;
  }

  RenderWeirdGradeint(Buffer, State->XOffset, State->YOffset);
  RenderPlayer(Buffer, State->PlayerX, State->PlayerY);
  RenderPlayer(Buffer, Input->MouseX, Input->MouseY);

  for(int i = 0; i < ArrayCount(Input->MouseButtons); i++) {
    if(Input->MouseButtons[i].IsEndedDown) {
      RenderPlayer(Buffer, 10, 50 + 10 * i);
    }
  }
}

extern "C" GAME_UPDATE_AUDIO(GameUpdateAudio) {
  game_state *State = (game_state *)Memory->PermanentStorage;
  RenderSineWave(State, SoundBuffer);
}
