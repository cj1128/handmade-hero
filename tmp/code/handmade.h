#ifndef HANDMADE_H
#define HANDMADE_H

#include "handmade_platform.h"
#include "handmade_random.h"
#include "handmade_math.h"

#define internal static
#define local_persist static
#define global_variable static

#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))

//TODO: Implements sinf by myself
#define Pi32 3.14159265359f

#define ArrayCount(Array) (sizeof((Array)) / sizeof((Array)[0]))

#if HANDMADE_DEBUG
#define Assert(expression) if(!(expression)){ *(int *)0 = 0; }
#else
#define Assert(expression)
#endif

#define Kilobytes(value) ((value)*1024LL)
#define Megabytes(value) (Kilobytes(value)*1024LL)
#define Gigabytes(value) (Megabytes(value)*1024LL)
#define Terabytes(value) (Gigabytes(value)*1024LL)



inline game_controller_input *GetController(game_input *Input, uint32 Index)
{
    Assert(Index < ArrayCount(Input->Controllers));
    game_controller_input *Result = &Input->Controllers[Index];
    return Result;
}


struct memory_arena
{
    uint8 *Base;
    uint64 Used;
    uint64 Size;
};


#define PushSize(Arena, Type) (Type *)PushSize_(Arena, sizeof(Type))
#define PushArray(Arena, Count, Type) (Type *)PushSize_(Arena, (Count) * sizeof(Type))

internal uint8 *
PushSize_(memory_arena *Arena, uint32 Size)
{
    Assert(Arena->Used + Size <= Arena->Size);
    uint8 *Result = Arena->Base + Arena->Used;
    Arena->Used += Size;
    return Result;
}


#include "handmade_intrinsics.h"
#include "handmade_tile.h"

struct world
{
    tile_map *TileMap;
};

struct loaded_bitmap
{
    uint32 *Pixels;
    int32 Width;
    int32 Height;
};

struct hero_bitmaps
{
    loaded_bitmap Head;
    loaded_bitmap Cape;
    loaded_bitmap Torso;
    v2 Align;
};

struct entity
{
    bool32 Exists;
    tile_map_pos P;
    v2 dP;
    uint32 FacingDirection;
    real32 Height;
    real32 Width;
};

struct game_state
{
    tile_map_pos CameraP;
    world *World;
    entity Entities[256];
    uint32 PlayerIndexForController[ArrayCount(((game_input *)0)->Controllers)];
    uint32 CameraFollowingEntityIndex;
    uint32 EntityCount;

    loaded_bitmap Backdrop;
    hero_bitmaps HeroBitmaps[4];
};

#endif
