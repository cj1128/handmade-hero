/*
* @Author: dingxijin
* @Date:   2015-07-23 11:32:11
* @Last Modified by:   dingxijin
* @Last Modified time: 2015-07-23 11:48:49
*/
#ifndef HANDMADE_INTRINSICS_H

#include <math.h>

inline real32
SquareRoot(real32 Value)
{
    real32 Result = sqrtf(Value);
    return Result;
}

inline real32
AbsoluteValue(real32 Value)
{
    real32 Result = fabs(Value);
    return Result;
}

inline int32
RoundReal32ToInt32(real32 Value)
{
  int32 Result = (int32)(roundf(Value));
  return Result;
}

inline uint32
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

inline uint32
SafeTruncateUint64(uint64 Value)
{
  Assert( Value <= 0xffffffff );
  uint32 Result = (uint32)Value;
  return Result;
}


struct bit_scan_result
{
    bool32 Found;
    uint32 Index;
};

inline bit_scan_result
BitScanForward(uint32 Value)
{
    bit_scan_result Result = {};
#ifdef COMPILER_MSVC
    Result.Found = _BitScanForward((unsigned long *)&Result.Index, Value);
#else
    for(int Index = 0;
        Index < 32;
        Index++)
    {
        if(Value & (1 << Index))
        {
            Result.Found = true;
            Result.Index = Index;
            break;
        }
    }
#endif
    return Result;
}


#define HANDMADE_INTRINSICS_H
#endif
