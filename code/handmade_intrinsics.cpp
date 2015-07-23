/*
* @Author: dingxijin
* @Date:   2015-07-23 11:32:11
* @Last Modified by:   dingxijin
* @Last Modified time: 2015-07-23 11:48:49
*/

internal int32
RoundReal32ToInt32(real32 Value)
{
  int32 Result = (int32)(roundf(Value));
  return Result;
}

internal uint32
RoundReal32ToUInt32(real32 Value)
{
  uint32 Result = (uint32)(Value + 0.5f);
  return Result;
}

inline int32
FloorReal32ToInt32(real32 Value)
{
  int32 Result = (int32)(floor(Value));
  return Result;
}

uint32
SafeTruncateUint64(uint64 Value)
{
  Assert( Value <= 0xffffffff );
  uint32 Result = (uint32)Value;
  return Result;
}
