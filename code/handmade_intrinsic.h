#ifndef HANDMADE_INTRINSIC_H
#define HANDMADE_INTRINSIC_H

#include "math.h"

inline f32
SquareRoot(f32 value)
{
  f32 result = sqrtf(value);
  return result;
}

inline i32
RoundReal32ToUint32(f32 input)
{
  u32 result = (u32)roundf(input);
  return result;
}

inline i32
RoundReal32ToInt32(f32 input)
{
  i32 result = (i32)roundf(input);
  return result;
}

inline i32
FloorReal32ToInt32(f32 value)
{
  i32 result = (i32)floor(value);
  return result;
}

inline i32
CeilReal32ToInt32(f32 value)
{
  i32 result = (i32)ceil(value);
  return result;
}

inline f32
Sin(f32 value)
{
  f32 result = sinf(value);
  return result;
}

inline f32
Cos(f32 value)
{
  f32 result = cosf(value);
  return result;
}

struct bit_scan_result {
  bool32 found;
  u32 index;
};

inline bit_scan_result
FindLeastSignificantSetBit(u32 value)
{
  bit_scan_result result = {};

#if COMPILER_MSVC
  result.found = _BitScanForward((unsigned long *)&result.index, value);
#else
  u32 test = 0;
  for(u32 test = 0; test < 32; test++) {
    if((value & (1 << test)) == 1) {
      result.found = true;
      result.index = test;
    }
  }
#endif

  return result;
}

inline f32
Square(f32 value)
{
  f32 result;
  result = value * value;
  return result;
}

inline f32
Abs(f32 value)
{
  f32 result = fabsf(value);
  return result;
}

inline u32
RotateLeft(u32 value, i32 shift)
{
  u32 result = _rotl(value, shift);
  return result;
}

inline u32
RotateRight(u32 value, i32 shift)
{
  u32 result = _rotr(value, shift);
  return result;
}

#endif
