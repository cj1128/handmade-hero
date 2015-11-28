/*
* @Author: dingxijin
* @Date:   2015-07-23 11:01:27
* @Last Modified by:   dingxijin
* @Last Modified time: 2015-07-27 00:18:48
*/

inline tile_chunk *
GetTileChunk(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
  tile_chunk *Result = 0;

  uint32 TileChunkX = AbsTileX >> TileMap->ChunkShift;
  uint32 TileChunkY = AbsTileY >> TileMap->ChunkShift;

  Assert(AbsTileZ >= 0 && AbsTileZ <= 1);

  if(TileChunkX >= 0
    && TileChunkX < TileMap->TileChunkCountX
    && TileChunkY >= 0
    && TileChunkY < TileMap->TileChunkCountY)
  {
      Result = &TileMap->TileChunks[AbsTileZ * TileMap->TileChunkCountX * TileMap->TileChunkCountY + TileChunkY * TileMap->TileChunkCountX + TileChunkX];
  }
  return Result;
}


inline uint32
GetTileValue(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
  uint32 Result = 0;
  tile_chunk *TileChunk = GetTileChunk(TileMap, AbsTileX, AbsTileY, AbsTileZ);
  uint32 RelTileX = AbsTileX & TileMap->ChunkMask;
  uint32 RelTileY = AbsTileY & TileMap->ChunkMask;
  if(TileChunk && TileChunk->Tiles)
  {
    Result = TileChunk->Tiles[RelTileY * TileMap->ChunkDim + RelTileX];
  }
  return Result;
}

inline uint32
GetTileValue(tile_map *TileMap, tile_map_pos Pos)
{
  uint32 Result = GetTileValue(TileMap, Pos.AbsTileX, Pos.AbsTileY, Pos.AbsTileZ);
  return Result;
}


internal void
SetTileValue(memory_arena *WorldArena, tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ, uint32 Value)
{
    tile_chunk *TileChunk = GetTileChunk(TileMap, AbsTileX, AbsTileY, AbsTileZ);
    uint32 RelTileX = AbsTileX & TileMap->ChunkMask;
    uint32 RelTileY = AbsTileY & TileMap->ChunkMask;
    if(TileChunk && !TileChunk->Tiles)
    {
        int TileCount = TileMap->ChunkDim * TileMap->ChunkDim;
        TileChunk->Tiles = PushArray(WorldArena, TileCount, uint32);
        for(int TileIndex = 0;
            TileIndex < TileCount;
            TileIndex++)
        {
            TileChunk->Tiles[TileIndex] = 1;
        }
    }
    if(TileChunk)
    {
      TileChunk->Tiles[RelTileY * TileMap->ChunkDim + RelTileX] = Value;
    }
}

inline void
RecanonicalizeCoord(tile_map *TileMap, uint32 *AbsTile, real32 *TileRel)
{
  int32 Offset = RoundReal32ToInt32((*TileRel) / TileMap->TileSideInMeters);
  *AbsTile += Offset;
  *TileRel -= Offset * TileMap->TileSideInMeters;
  Assert(*TileRel >= -0.5f * TileMap->TileSideInMeters);
  Assert(*TileRel <= 0.5f * TileMap->TileSideInMeters);
}


internal tile_map_pos
RecanonicalizePos(tile_map *TileMap, tile_map_pos Pos)
{
  tile_map_pos Result = Pos;
  RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.Offset.X);
  RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.Offset.Y);
  return Result;
}


inline bool32
IsTileMapPointValid(tile_map *TileMap, tile_map_pos Pos)
{
  bool32 Result = false;
  int32 TileValue = GetTileValue(TileMap, Pos.AbsTileX, Pos.AbsTileY, Pos.AbsTileZ);
  if(TileValue == 1 ||
     TileValue == 3 ||
     TileValue == 4)
  {
    Result = true;
  }
  return Result;
}

inline bool32
AreOnSameTile(tile_map_pos *A, tile_map_pos *B)
{
  bool32 Result = (A->AbsTileX == B->AbsTileX &&
                 A->AbsTileY == B->AbsTileY &&
                 A->AbsTileZ == B->AbsTileZ);
  return Result;
}

inline tile_map_difference
Subtract(tile_map *TileMap, tile_map_pos *Left, tile_map_pos *Right)
{
    tile_map_difference Result = {};

    // TODO: we may need to figure out what dz means
    Result.dZ = 0;
    Result.dXY = {};

    Result.dXY = Left->Offset - Right->Offset;
    v2 TiledXY= {((real32)Left->AbsTileX - (real32)Right->AbsTileX) * TileMap->TileSideInMeters,
                 ((real32)Left->AbsTileY - (real32)Right->AbsTileY) * TileMap->TileSideInMeters};

    Result.dXY += TiledXY;

    return Result;
}

inline tile_map_position
CenteredTilePoint(AbsTileX, AbsTileY, AbsTileZ)
{
    tile_map_position Result = {};

    Result.AbsTileX = AbsTileX;
    Result.AbsTileY = AbsTileY;
    Result.AbsTileZ = AbsTileZ;

    return Result;
}

