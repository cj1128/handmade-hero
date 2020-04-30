#ifndef HANDMADE_MATH_H
#define HANDMADE_MATH_H

//
// v2
//

union v2 {
  struct {
    real32 x, y;
  };
  real32 e[2];
};

inline v2
operator+(v2 a, v2 b) {
  v2 result;
  result.x = a.x + b.x;
  result.y = a.y + b.y;
  return result;
}

inline v2
operator+=(v2 &a, v2 b) {
  a = a + b;
  return a;
}

inline v2
operator-(v2 a) {
  v2 result;
  result.x = -a.x;
  result.y = -a.y;
  return result;
}

inline v2
operator-(v2 a, v2 b) {
  v2 result;
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  return result;
}

inline v2
operator-=(v2 &a, v2 b) {
  a = a - b;
  return a;
}

inline v2
operator*(real32 a, v2 b) {
  v2 result = {a * b.x, a * b.y};
  return result;
}

inline v2
operator*(v2 b, real32 a) {
  v2 result = a * b;
  return result;
}

inline v2
operator*=(v2 &a, real32 b) {
  a = b * a;
  return a;
}

inline real32
Inner(v2 a, v2 b) {
  real32 result = a.x * b.x + a.y * b.y;
  return result;
}

inline v2
Hadamard(v2 a, v2 b) {
  v2 result = {a.x * b.x, a.y * b.y};
  return result;
}

inline real32
LengthSq(v2 a) {
  real32 result = Inner(a, a);
  return result;
}

inline real32
Length(v2 a) {
  real32 result = SquareRoot(LengthSq(a));
  return result;
}

inline bool32
IsZero(v2 a) {
  bool32 result = a.x == 0 && a.y == 0;
  return result;
}

//
// v3
//

union v3 {
  struct {
    real32 x, y, z;
  };
  struct {
    real32 r, g, b;
  };
  struct {
    v2 xy;
    real32 _ignored;
  };
  real32 e[2];
};

inline v3
V3(v2 xy, real32 z) {
  v3 result = {xy.x, xy.y, z};
  return result;
}

inline v3
operator+(v3 a, v3 b) {
  v3 result;
  result.x = a.x + b.x;
  result.y = a.y + b.y;
  result.z = a.z + b.z;
  return result;
}

inline v3
operator+=(v3 &a, v3 b) {
  a = a + b;
  return a;
}

inline v3
operator-(v3 a) {
  v3 result;
  result.x = -a.x;
  result.y = -a.y;
  result.z = -a.z;
  return result;
}

inline v3
operator-(v3 a, v3 b) {
  v3 result;
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  result.z = a.z - b.z;
  return result;
}

inline v3
operator-=(v3 &a, v3 b) {
  a = a - b;
  return a;
}

inline v3
operator*(real32 a, v3 b) {
  v3 result = {a * b.x, a * b.y, a * b.z};
  return result;
}

inline v3
operator*(v3 b, real32 a) {
  v3 result = a * b;
  return result;
}

inline v3
operator*=(v3 &a, real32 b) {
  a = b * a;
  return a;
}

inline real32
Inner(v3 a, v3 b) {
  real32 result = a.x * b.x + a.y * b.y + a.z * b.z;
  return result;
}

inline v3
Hadamard(v3 a, v3 b) {
  v3 result = {a.x * b.x, a.y * b.y, a.z * b.z};
  return result;
}

inline real32
LengthSq(v3 a) {
  real32 result = Inner(a, a);
  return result;
}

inline real32
Length(v3 a) {
  real32 result = SquareRoot(LengthSq(a));
  return result;
}

inline bool32
IsZero(v3 a) {
  bool32 result = a.x == 0 && a.y == 0 && a.z == 0;
  return result;
}

//
// v4
//

union v4 {
  struct {
    real32 x, y, z, w;
  };
  struct {
    real32 r, g, b, a;
  };
  real32 e[2];
};

//
// rectangle2
//

struct rectangle2 {
  v2 min;
  v2 max;
};

inline rectangle2
RectMinMax(v2 min, v2 max) {
  rectangle2 result = {};
  result.min = min;
  result.max = max;
}

inline rectangle2
RectCenterHalfDim(v2 center, v2 halfDim) {
  rectangle2 result = {};
  result.min = center - halfDim;
  result.max = center + halfDim;
  return result;
}

inline rectangle2
RectCenterDim(v2 center, v2 dim) {
  rectangle2 result = RectCenterHalfDim(center, 0.5f * dim);
  return result;
}

inline rectangle2
AddRadius(rectangle2 rect, v2 radius) {
  rectangle2 result = rect;
  result.min -= radius;
  result.max += radius;
  return result;
}

inline bool32
IsInRectangle(rectangle2 rect, v2 test) {
  bool32 result = (test.x >= rect.min.x) && (test.x <= rect.max.x) &&
                  (test.y >= rect.min.y) && (test.y <= rect.max.y);
  return result;
}

//
// rectangle3

struct rectangle3 {
  v3 min;
  v3 max;
};

inline rectangle3
RectMinMax(v3 min, v3 max) {
  rectangle3 result = {};
  result.min = min;
  result.max = max;
}

inline rectangle3
RectCenterHalfDim(v3 center, v3 halfDim) {
  rectangle3 result = {};
  result.min = center - halfDim;
  result.max = center + halfDim;
  return result;
}

inline rectangle3
RectCenterDim(v3 center, v3 dim) {
  rectangle3 result = RectCenterHalfDim(center, 0.5f * dim);
  return result;
}

inline rectangle3
AddRadius(rectangle3 rect, v3 radius) {
  rectangle3 result = rect;
  result.min -= radius;
  result.max += radius;
  return result;
}

inline bool32
IsInRectangle(rectangle3 rect, v3 test) {
  bool32 result = (test.x >= rect.min.x) && (test.x <= rect.max.x) &&
                  (test.y >= rect.min.y) && (test.y <= rect.max.y) &&
                  (test.z >= rect.min.z) && (test.z <= rect.max.z);
  return result;
}

#endif
