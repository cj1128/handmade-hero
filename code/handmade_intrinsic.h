#ifndef HANDMADE_INTRINSIC_H
#define HANDMADE_INTRINSIC_H

#include "math.h"

inline real32
SquareRoot(real32 value)
{
  real32 result = sqrtf(value);
  return result;
}

inline int32
RoundReal32ToUint32(real32 input)
{
  uint32 result = (uint32)roundf(input);
  return result;
}

inline int32
RoundReal32ToInt32(real32 input)
{
  int32 result = (int32)roundf(input);
  return result;
}

inline int32
FloorReal32ToInt32(real32 value)
{
  int32 result = (int32)floor(value);
  return result;
}

inline int32
CeilReal32ToInt32(real32 value)
{
  int32 result = (int32)ceil(value);
  return result;
}

inline real32
Sin(real32 value)
{
  real32 result = sinf(value);
  return result;
}

inline real32
Cos(real32 value)
{
  real32 result = cosf(value);
  return result;
}

struct bit_scan_result {
  bool32 found;
  uint32 index;
};

inline bit_scan_result
FindLeastSignificantSetBit(uint32 value)
{
  bit_scan_result result = {};

#if COMPILER_MSVC
  result.found = _BitScanForward((unsigned long *)&result.index, value);
#else
  uint32 test = 0;
  for(uint32 test = 0; test < 32; test++) {
    if((value & (1 << test)) == 1) {
      result.found = true;
      result.index = test;
    }
  }
#endif

  return result;
}

inline real32
Square(real32 value)
{
  real32 result;
  result = value * value;
  return result;
}

inline real32
Abs(real32 value)
{
  real32 result = fabsf(value);
  return result;
}

inline uint32
RotateLeft(uint32 value, int32 Shift)
{
  uint32 result = _rotl(value, Shift);
  return result;
}

inline uint32
RotateRight(uint32 value, int32 Shift)
{
  uint32 result = _rotr(value, Shift);
  return result;
}

#endif
