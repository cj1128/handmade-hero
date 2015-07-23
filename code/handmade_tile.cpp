/*
* @Author: dingxijin
* @Date:   2015-07-23 11:01:27
* @Last Modified by:   dingxijin
* @Last Modified time: 2015-07-23 12:23:46
*/

inline tile_chunk *
GetTileChunk(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY)
{
  tile_chunk *Result = 0;

  uint32 TileChunkX = AbsTileX >> TileMap->ChunkShift;
  uint32 TileChunkY = AbsTileY >> TileMap->ChunkShift;

  if(TileChunkX >= 0 && TileChunkX < TileMap->TileChunkCountX
    && TileChunkY >= 0 && TileChunkY < TileMap->TileChunkCountY)
  {
      Result = &TileMap->TileChunks[TileChunkY * TileMap->TileChunkCountX + TileChunkX];
  }
  return Result;
}


inline uint32
GetTileValue(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY)
{
  uint32 Result = 0;
  tile_chunk *TileChunk = GetTileChunk(TileMap, AbsTileX, AbsTileY);
  uint32 RelTileX = AbsTileX & TileMap->ChunkMask;
  uint32 RelTileY = AbsTileY & TileMap->ChunkMask;
  if(TileChunk)
  {
    Result = TileChunk->Tiles[RelTileY * TileMap->ChunkDim + RelTileX];
  }
  return Result;
}


inline void
SetTileValue(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 Value)
{
    tile_chunk *TileChunk = GetTileChunk(TileMap, AbsTileX, AbsTileY);
    uint32 RelTileX = AbsTileX & TileMap->ChunkMask;
    uint32 RelTileY = AbsTileY & TileMap->ChunkMask;
    if(TileChunk)
    {
      TileChunk->Tiles[RelTileY * TileMap->ChunkDim + RelTileX] = Value;
    }
}

internal void
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
  RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.TileRelX);
  RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.TileRelY);
  return Result;
}

internal bool
IsWorldPointValid(tile_map *TileMap, tile_map_pos Pos)
{
  bool Result = false;
  if(GetTileValue(TileMap, Pos.AbsTileX, Pos.AbsTileY) == 0)
  {
    Result = true;
  }
  return Result;
}
