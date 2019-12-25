#ifndef HANDMADE_INTRINSIC_H

#include "math.h"

inline int32
RoundReal32ToUint32(real32 Input) {
  uint32 Result = (uint32)roundf(Input);
  return Result;
}

inline int32
RoundReal32ToInt32(real32 Input) {
  int32 Result = (int32)roundf(Input);
  return Result;
}

inline int32
FloorReal32ToInt32(real32 Value) {
  int32 Result = (int32)floor(Value);
  return Result;
}

inline real32
Sin(real32 Value) {
  real32 Result = sinf(Value);
  return Result;
}

inline real32
Cos(real32 Value) {
  real32 Result = cosf(Value);
  return Result;
}

struct bit_scan_result {
  bool32 Found;
  uint32 Index;
};

inline bit_scan_result
FindLeastSignificantSetBit(uint32 Value) {
  bit_scan_result Result = {};

#if COMPILER_MSVC
  Result.Found = _BitScanForward((unsigned long*)&Result.Index, Value);
#else
  uint32 Test = 0;
  for(uint32 Test = 0; Test < 32; Test++) {
    if((Value & (1 << Test)) == 1) {
      Result.Found = true;
      Result.Index = Test;
    }
  }
#endif

  return Result;
}

inline real32
Square(real32 Value) {
  real32 Result;
  Result = Value * Value;
  return Result;
}

inline real32
AbsoluteValue(real32 Value) {
  real32 Result = fabsf(Value);
  return Result;
}

inline uint32
RotateLeft(uint32 Value, int32 Shift) {
  uint32 Result = _rotl(Value, Shift);
  return Result;
}

inline uint32
RotateRight(uint32 Value, int32 Shift) {
  uint32 Result = _rotr(Value, Shift);
  return Result;
}

#define HANDMADE_INTRINSIC_H
#endif
