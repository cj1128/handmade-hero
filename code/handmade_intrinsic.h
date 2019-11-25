#ifndef HANDMADE_INTRINSIC

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

#define HANDMADE_INTRINSIC
#endif
