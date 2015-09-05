/*
 * @Author: dingxijin
 * @Date:   2015-04-21 08:09:18
 * @Last Modified by:   dingxijin
 * @Last Modified time: 2015-07-27 00:23:22
 */

#include "handmade.h"
#include "handmade_tile.cpp"

internal void
UpdateSound(game_sound_buffer *SoundBuffer, int ToneHz)
{
    local_persist float tSine;
    int ToneVolume = 10000;
    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;
    int16_t *SampleOut = SoundBuffer->Samples;
    for(int SampleIndex = 0 ;
        SampleIndex < SoundBuffer->SampleCount ;
        SampleIndex++)
    {
        float SineValue = sinf(tSine);
        int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
#if 0
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;
#else
        *SampleOut = 0;
#endif
        tSine += 2.0f * Pi32 / (float)(WavePeriod);
        if( tSine > 2.0f * Pi32 )
        {
            tSine -= 2.0f * Pi32;
        }
    }
}

internal void
DrawBitmap(game_offscreen_buffer *Buffer, loaded_bitmap *Bitmap,
           v2 Start, v2 Align = {})

{
    int32 MinX = RoundReal32ToInt32(Start.X);
    int32 MinY = RoundReal32ToInt32(Start.Y);

    MinX -= (int32)Align.X;
    MinY -= (int32)Align.Y;


    int32 MaxX = MinX + Bitmap->Width;
    int32 MaxY = MinY + Bitmap->Height;

    int32 SourceOffsetX = 0;
    int32 SourceOffsetY = 0;

    if(MinX < 0)
    {
        SourceOffsetX = -MinX;
        MinX = 0;
    }
    if(MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }
    if(MinY < 0)
    {
        SourceOffsetY = -MinY;
        MinY = 0;
    }
    if(MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }
    uint32 *SourceRow = Bitmap->Pixels + Bitmap->Width * (Bitmap->Height -1) - SourceOffsetY * Bitmap->Width + SourceOffsetX;
    uint32 *DestRow = (uint32 *)Buffer->Memory + MinY * Buffer->Width + MinX;
    for(int Row = MinY; Row < MaxY; Row++)
    {
        for(int Column = MinX; Column < MaxX; Column++)
        {
            int Offset = Column - MinX;

            uint32 *Source = SourceRow + Offset;
            uint32 *Dest = DestRow + Offset;

            real32 A = (*Source >> 24) / 255.0f;

            uint8 SR = (*Source >> 16) & 0xff;
            uint8 SG = (*Source >> 8) & 0xff;
            uint8 SB = (*Source >> 0) & 0xff;

            uint8 DR = (*Dest >> 16) & 0xff;
            uint8 DG = (*Dest >> 8) & 0xff;
            uint8 DB = (*Dest >> 0) & 0xff;

            real32 R = (1 - A)*DR + SR * A;
            real32 G = (1 - A)*DG + SG * A;
            real32 B = (1 - A)*DB + SB * A;

            *Dest = (((uint32)(R + 0.5f) << 16) |
                     ((uint32)(G + 0.5f) << 8) |
                     ((uint32)(B + 0.5f) << 0));
                     
        }
        SourceRow -= Bitmap->Width;
        DestRow += Buffer->Width;
    }
}

internal void
DrawRect(game_offscreen_buffer *Buffer, v2 Min, v2 Max,
         real32 R, real32 G, real32 B)
{
    int32 IntMinX = RoundReal32ToInt32(Min.X);
    int32 IntMaxX = RoundReal32ToInt32(Max.X);
    int32 IntMinY = RoundReal32ToInt32(Min.Y);
    int32 IntMaxY = RoundReal32ToInt32(Max.Y);

    uint32 IntR = RoundReal32ToUInt32(R * 255.0f);
    uint32 IntG = RoundReal32ToUInt32(G * 255.0f);
    uint32 IntB = RoundReal32ToUInt32(B * 255.0f);

    uint32 Color = (IntR << 16) |
        (IntG << 8) |
        (IntB);

    if(IntMinX < 0)
    {
        IntMinX = 0;
    }
    if(IntMaxX > Buffer->Width)
    {
        IntMaxX = Buffer->Width;
    }
    if(IntMinY < 0)
    {
        IntMinY = 0;
    }
    if(IntMaxY > Buffer->Height)
    {
        IntMaxY = Buffer->Height;
    }
    for(int Row = IntMinY; Row < IntMaxY; Row++)
    {
        for(int Column = IntMinX; Column < IntMaxX; Column++)
        {
            uint32 *Pixel = (uint32 *)((uint8 *)Buffer->Memory + Row * Buffer->Pitch + Column * Buffer->BytesPerPixel);
            *Pixel = Color;
        }
    }
}

/*
  internal void
  RenderWeirdGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
  {
  uint8 *Row = (uint8 *)Buffer->Memory;

  for(int Y = 0;
  Y < Buffer->Height;
  ++Y)
  {
  uint32 *Pixel = (uint32 *)Row;
  for(int XX = 0;
  X < Buffer->Width;
  ++X)
  {
  // Memory Order: BB GG RR XX
  // 0xXXRRGGBB
  uint8 Blue = (uint8)(X + BlueOffset);
  uint8 Green = (uint8)(Y + GreenOffset);

  *Pixel++ = ((Green << 16) | Blue);

  }
  Row += Buffer->Pitch;
  }
  }
*/

internal void
RenderPlayer(game_offscreen_buffer *Buffer, int PlayerX, int PlayerY)
{
    uint8 *EndOfBuffer = (uint8 *)Buffer->Memory + Buffer->Pitch * Buffer->Height;
    uint32 Color = 0xABCD12FF;
    int Top = PlayerY;
    int Bottom = PlayerY + 10;
    for(int X = PlayerX;
        X < PlayerX + 10;
        X++)
    {
        uint8 *Pixel = (uint8 *)Buffer->Memory +
            X * Buffer->BytesPerPixel + Buffer->Pitch * Top;
        for(int Y = Top;
            Y <= Bottom;
            Y++)
        {
            if((Pixel >= Buffer->Memory) &&
               ((Pixel + 4) <= EndOfBuffer))
            {
                *(uint32 *)Pixel = Color;
            }
            Pixel += Buffer->Pitch;
        }
    }
}

#pragma pack(push, 1)
struct bitmap_header
{
    uint16   FileType;     /* File type, always 4D42h ("BM") */
    uint32  FileSize;     /* Size of the file in bytes */
    uint16   Reserved1;    /* Always 0 */
    uint16   Reserved2;    /* Always 0 */
    uint32  BitmapOffset; /* Starting position of image data in bytes */
    uint32 Size;            /* Size of this header in bytes */
    int32  Width;           /* Image width in pixels */
    int32  Height;          /* Image height in pixels */
    uint16  Planes;          /* Number of color planes */
    uint16  BitsPerPixel;    /* Number of bits per pixel */
    int32 Compression;
    int32 SizeOfBitmap;    /* Size of bitmap in bytes */
    int32  HorzResolution;  /* Horizontal resolution in pixels per meter */
    int32  VertResolution;  /* Vertical resolution in pixels per meter */
    int32 ColorsUsed;      /* Number of colors in the image */
    int32 ColorsImportant; /* Minimum number of important colors */
    uint32 RedMask;
    uint32 GreenMask;
    uint32 BlueMask;
};
#pragma pack(pop)


internal loaded_bitmap
DEBUGLoadBMP(thread_context *Thread, debug_platform_read_file *ReadFile, char *FileName)
{
    loaded_bitmap Result = {};
    debug_read_file_result ReadContent = ReadFile(Thread, FileName);
    bitmap_header *Header = (bitmap_header *)ReadContent.Content;

    Assert(Header->Compression == 3);

    uint32 AlphaMask = ~(Header->RedMask |
                        Header->GreenMask |
                        Header->BlueMask);
    Result.Width = Header->Width;
    Result.Height = Header->Height;

    if(ReadContent.ContentSize != 0)
    {
        Result.Pixels = (uint32 *)((uint8 *)ReadContent.Content + Header->BitmapOffset);
    }
    uint32 *Pointer = Result.Pixels;
    for(int Y = 0;
        Y < Header->Height;
        Y++)
    {
        for(int X = 0;
            X < Header->Width;
            X++)
        {
            uint32 RedShift = BitScanForward(Header->RedMask).Index;
            uint32 GreenShift = BitScanForward(Header->GreenMask).Index;
            uint32 BlueShift = BitScanForward(Header->BlueMask).Index;
            uint32 AlphaShift = BitScanForward(AlphaMask).Index;

            uint32 R = *Pointer & Header->RedMask;
            uint32 G = *Pointer & Header->GreenMask;
            uint32 B = *Pointer & Header->BlueMask;
            uint32 A = *Pointer & AlphaMask;

            *Pointer = (((A >> AlphaShift) << 24) |
                        ((R >> RedShift) << 16) |
                        ((G >> GreenShift) << 8) |
                        (B >> BlueShift));
            Pointer++;
        }
    }
    return Result;
}



extern "C" GAME_UPDATE_VIDEO(GameUpdateVideo)
{

    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    game_state *GameState = (game_state *)Memory->PermanentStorage;



    if(!Memory->IsInitialized)
    {
        GameState->Backdrop = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadFile, "test/test_background.bmp");
        GameState->HeroFacingDirection = 2;
        hero_bitmaps *HeroBitmaps = GameState->HeroBitmaps;

        HeroBitmaps->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadFile, "test/test_hero_back_head.bmp");
        HeroBitmaps->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadFile, "test/test_hero_back_cape.bmp");
        HeroBitmaps->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadFile, "test/test_hero_back_torso.bmp");
        HeroBitmaps->Align = {72, 180};

        HeroBitmaps++;
        HeroBitmaps->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadFile, "test/test_hero_right_head.bmp");
        HeroBitmaps->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadFile, "test/test_hero_right_cape.bmp");
        HeroBitmaps->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadFile, "test/test_hero_right_torso.bmp");
        HeroBitmaps->Align = {72, 180};

        HeroBitmaps++;
        HeroBitmaps->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadFile, "test/test_hero_front_head.bmp");
        HeroBitmaps->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadFile, "test/test_hero_front_cape.bmp");
        HeroBitmaps->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadFile, "test/test_hero_front_torso.bmp");
        HeroBitmaps->Align = {72, 180};


        HeroBitmaps++;
        HeroBitmaps->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadFile, "test/test_hero_left_head.bmp");
        HeroBitmaps->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadFile, "test/test_hero_left_cape.bmp");
        HeroBitmaps->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadFile, "test/test_hero_left_torso.bmp");
        HeroBitmaps->Align = {72, 180};

        Memory->IsInitialized = true;

        GameState->PlayerP.AbsTileX = 3;
        GameState->PlayerP.AbsTileY = 3;
        GameState->PlayerP.AbsTileZ = 0;
        GameState->PlayerP.Offset = { 1.0, 1.0 };

        GameState->CameraP.AbsTileX = 17 / 2;
        GameState->CameraP.AbsTileY = 9 / 2;
        GameState->CameraP.AbsTileZ = 0;


        memory_arena WorldArena = {};
        WorldArena.Base = (uint8 *)Memory->PermanentStorage + sizeof(game_state);
        WorldArena.Size = Memory->PermanentStorageSize - sizeof(game_state);
        WorldArena.Used = 0;

        GameState->World = PushSize(&WorldArena, world);
        world *World = GameState->World;
        World->TileMap = PushSize(&WorldArena, tile_map);

        tile_map *TileMap = World->TileMap;

        TileMap->ChunkShift = 4;
        TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
        TileMap->ChunkDim = 1 << TileMap->ChunkShift;

        TileMap->TileChunkCountX = 64;
        TileMap->TileChunkCountY = 64;
        TileMap->TileChunkCountZ = 2;

        TileMap->TileSideInMeters = 1.4f;

        TileMap->TileChunks = PushArray(&WorldArena, TileMap->TileChunkCountY * TileMap->TileChunkCountX * TileMap->TileChunkCountZ, tile_chunk);

        int TilePerWidth = 17;
        int TilePerHeight = 9;

        int ScreenX = 0;
        int ScreenY = 0;

        bool32 DoorUp = false;
        bool32 DoorDown = false;
        bool32 DoorTop = false;
        bool32 DoorBottom = false;
        bool32 DoorLeft = false;
        bool32 DoorRight = false;
        bool32 CreateZDoor = false;

        uint32 AbsTileZ = 0;

        for(int ScreenIndex = 0; ScreenIndex < 20; ScreenIndex++)
        {
            int RandomChoice;
            CreateZDoor = false;

            if(DoorUp || DoorDown)
            {
                RandomChoice = rand() % 2;
            }
            else
            {
                RandomChoice = rand() % 3;
            }
            if(RandomChoice == 2)
            {
                CreateZDoor = true;
                if(AbsTileZ == 0)
                {
                    DoorUp = true;
                }
                else if(AbsTileZ == 1)
                {
                    DoorDown = true;
                }
            }
            if(RandomChoice == 0)
            {
                DoorRight = true;
            }
            if(RandomChoice == 1)
            {
                DoorBottom = true;
            }

            for(int TileY = 0; TileY < TilePerHeight; TileY++)
            {
                for(int TileX = 0; TileX < TilePerWidth; TileX++)
                {
                    uint32 TileValue = 1;
                    if(TileX == 0 && (!DoorLeft || (TileY != (TilePerHeight / 2))) )
                    {
                        TileValue = 2;
                    }
                    if(TileX == (TilePerWidth -1) && (!DoorRight || (TileY != (TilePerHeight / 2))))
                    {
                        TileValue = 2;
                    }
                    if(TileY == 0 && (!DoorTop || (TileX != (TilePerWidth / 2))))
                    {
                        TileValue = 2;
                    }
                    if(TileY == (TilePerHeight -1) && (!DoorBottom || (TileX != (TilePerWidth /2))))
                    {
                        TileValue = 2;
                    }
                    if(TileX == 10 && TileY == 5)
                    {
                        if(DoorUp)
                        {
                            TileValue = 3;
                        }
                        if(DoorDown)
                        {
                            TileValue = 4;
                        }
                    }
                    SetTileValue(&WorldArena, TileMap, ScreenX * TilePerWidth + TileX,
                                 ScreenY * TilePerHeight + TileY, AbsTileZ, TileValue);
                }
            }

            DoorLeft = DoorRight;
            DoorTop = DoorBottom;

            DoorRight = false;
            DoorBottom = false;

            if(CreateZDoor)
            {
                if(DoorUp)
                {
                    DoorDown = true;
                    DoorUp = false;
                    AbsTileZ = 1;
                }
                else if(DoorDown)
                {
                    DoorUp = true;
                    DoorDown = false;
                    AbsTileZ = 0;
                }
            }
            else
            {
                DoorUp = false;
                DoorDown = false;
            }

            if(RandomChoice == 0)
            {
                ScreenX += 1;
            }
            else if(RandomChoice == 1)
            {
                ScreenY += 1;
            }
        }
    }

    world *World = GameState->World;
    tile_map *TileMap = World->TileMap;

    real32 TileSideInPixels = 60;
    real32 MetersToPixels = TileSideInPixels / TileMap->TileSideInMeters;

    int32 UpperLeftX = 30;
    int32 UpperLeftY = 30;

    real32 PlayerHeight = 1.0f;
    real32 PlayerWidth = 0.75f * PlayerHeight;

    for(int Index = 0 ;
        Index < ArrayCount(Input->Controllers) && Input->Controllers[Index].IsConnected;
        Index++)
    {
        game_controller_input *Controller = GetController(Input, Index);
        if(Controller->IsAnalog)
        {
        }
        else
        {
            v2 ddPlayerP = {};
            real32 Speed = 20;
            if(Controller->MoveLeft.EndedDown)
            {
                ddPlayerP.X = -1;
                GameState->HeroFacingDirection = 3;
            }
            if(Controller->MoveRight.EndedDown)
            {
                ddPlayerP.X = 1;
                GameState->HeroFacingDirection = 1;
            }
            if(Controller->MoveUp.EndedDown)
            {
                ddPlayerP.Y = -1;
                GameState->HeroFacingDirection = 0;
            }
            if(Controller->MoveDown.EndedDown)
            {
                ddPlayerP.Y = 1;
                GameState->HeroFacingDirection = 2;
            }
            if(Controller->ActionUp.EndedDown)
            {
                Speed = 50;
            }

            ddPlayerP *= Speed;
 
            if(ddPlayerP.X != 0 && ddPlayerP.Y != 0)
            {
                ddPlayerP *= 0.707106781185f;
            }

            //NOTE(CJ): ODE
            ddPlayerP += -2.5f * GameState->dPlayerP;

            tile_map_pos NewPlayerP = GameState->PlayerP;
            NewPlayerP.Offset = 0.5f * ddPlayerP * Square(Input->TimeForFrame) +
                GameState->dPlayerP * Input->TimeForFrame +
                NewPlayerP.Offset;
            GameState->dPlayerP = GameState->dPlayerP + ddPlayerP * Input->TimeForFrame;

            NewPlayerP = RecanonicalizePos(TileMap, NewPlayerP);

            tile_map_pos LeftPlayerP = NewPlayerP;
            LeftPlayerP.Offset.X -= PlayerWidth / 2;
            LeftPlayerP = RecanonicalizePos(TileMap, LeftPlayerP);

            tile_map_pos RightPlayerP = NewPlayerP;
            RightPlayerP.Offset.X += PlayerWidth / 2;
            RightPlayerP = RecanonicalizePos(TileMap, RightPlayerP);

            bool32 Collided = false;
            tile_map_pos CollidedP = {};
            if(!IsTileMapPointValid(TileMap, NewPlayerP)) {
                Collided = true;
                CollidedP = NewPlayerP;
            }
            if(!IsTileMapPointValid(TileMap, LeftPlayerP)) {
                Collided = true;
                CollidedP = LeftPlayerP;
            }
            if(!IsTileMapPointValid(TileMap, RightPlayerP)) {
                Collided = true;
                CollidedP = RightPlayerP;
            }

            if(Collided)
            {
                v2 r = {};
                if(CollidedP.AbsTileX < GameState->PlayerP.AbsTileX)
                {
                    r = v2{1, 0};
                }
                if(CollidedP.AbsTileX > GameState->PlayerP.AbsTileX)
                {
                    r = v2{1, 0};
                }
                if(CollidedP.AbsTileY < GameState->PlayerP.AbsTileY)
                {
                    r = v2{0, 1};
                }
                if(CollidedP.AbsTileY > GameState->PlayerP.AbsTileY)
                {
                    r = v2{0, 1};
                }
                GameState->dPlayerP = GameState->dPlayerP - 2 * Inner(r, GameState->dPlayerP) * r;
            }
            else
            {
                if(!AreOnSameTile(&GameState->PlayerP, &NewPlayerP))
                {
                    uint32 TileValue = GetTileValue(TileMap, NewPlayerP);
                    if(TileValue == 3)
                    {
                        NewPlayerP.AbsTileZ++;
                    }
                    else if(TileValue == 4)
                    {
                        NewPlayerP.AbsTileZ--;
                    }
                }
                GameState->PlayerP = NewPlayerP;
                GameState->CameraP.AbsTileZ = GameState->PlayerP.AbsTileZ;

                tile_map_difference Diff = Subtract(TileMap, &GameState->PlayerP, &GameState->CameraP);
                if(Diff.dXY.X > 9 * TileMap->TileSideInMeters)
                {
                    GameState->CameraP.AbsTileX += 17;
                }
                if(Diff.dXY.X < -9 * TileMap->TileSideInMeters)
                {
                    GameState->CameraP.AbsTileX -= 17;
                }
                if(Diff.dXY.Y > 5 * TileMap->TileSideInMeters)
                {
                    GameState->CameraP.AbsTileY += 9;
                }
                if(Diff.dXY.Y < -5 * TileMap->TileSideInMeters)
                {
                    GameState->CameraP.AbsTileY -= 9;
                }
            }
        }
    }

    DrawBitmap(Buffer,
               &GameState->Backdrop,
               V2(0, 0));
    //DrawRect(Buffer, 0.0f, (real32)Buffer->Width, 0.0f, (real32)Buffer->Height, 1.0f, 0.0f, 0.0f);

    v2 ScreenCenter = {(real32)Buffer->Width / 2, (real32)Buffer->Height / 2};

    for(int RelRow = -5; RelRow < 5; RelRow++)
    {
        for(int RelColumn = -10; RelColumn < 10; RelColumn++)
        {
            real32 gray = 0.5;
            uint32 Row = GameState->CameraP.AbsTileY + RelRow;
            uint32 Column = GameState->CameraP.AbsTileX + RelColumn;
            uint32 TileValue = GetTileValue(TileMap, Column, Row, GameState->CameraP.AbsTileZ);
            if(TileValue > 1)
            {
                if(TileValue == 2)
                {
                    gray = 1.0;
                }
                if(Row == GameState->CameraP.AbsTileY && Column == GameState->CameraP.AbsTileX)
                {
                    gray = 0.0;
                }
                if(TileValue == 3 || TileValue == 4)
                {
                    gray = 0.25;
                }
                v2 TileCen = {};
                TileCen.X = ScreenCenter.X + (RelColumn * TileSideInPixels) - MetersToPixels * GameState->CameraP.Offset.X;
                TileCen.Y = ScreenCenter.Y + (RelRow * TileSideInPixels) - MetersToPixels * GameState->CameraP.Offset.Y;

                v2 Min = TileCen - V2(0.5f * TileSideInPixels, 0.5f * TileSideInPixels);
                v2 Max = Min + V2(TileSideInPixels, TileSideInPixels);

                DrawRect(Buffer,
                         Min,
                         Max,
                         gray, gray, gray);
            }
        }
    }

    tile_map_difference Diff = Subtract(TileMap, &GameState->PlayerP, &GameState->CameraP);

    v2 PlayerGroundPosition = {};
    PlayerGroundPosition.X = ScreenCenter.X + Diff.dXY.X * MetersToPixels;
    PlayerGroundPosition.Y = ScreenCenter.Y + Diff.dXY.Y * MetersToPixels;

    v2 PlayerLeftTop = {PlayerGroundPosition.X - 0.5f * MetersToPixels * PlayerWidth,
                        PlayerGroundPosition.Y - 1.0f * MetersToPixels * PlayerHeight};
    v2 PlayerWidthHeight = {PlayerWidth, PlayerHeight};

    hero_bitmaps *HeroBitmaps = &GameState->HeroBitmaps[GameState->HeroFacingDirection];

    DrawRect(Buffer,
             PlayerLeftTop,
             PlayerLeftTop + PlayerWidthHeight * MetersToPixels,
             0.7f, 0.8f, 0.9f);

    DrawBitmap(Buffer,
               &HeroBitmaps->Torso,
               PlayerGroundPosition,
               HeroBitmaps->Align);

    DrawBitmap(Buffer,
               &HeroBitmaps->Cape,
               PlayerGroundPosition,
               HeroBitmaps->Align);
    DrawBitmap(Buffer,
               &HeroBitmaps->Head,
               PlayerGroundPosition,
               HeroBitmaps->Align);

}

extern "C" GAME_UPDATE_AUDIO(GameUpdateAudio)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    UpdateSound(SoundBuffer, 40);
}
