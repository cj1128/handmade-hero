#ifndef HANDMADE_MATH_H
#define HANDMADE_MATH_H

union v2 {
  struct {
    real32 x, y;
  };
  real32 e[2];
};

union v3 {
  struct {
    real32 x, y, z;
  };
  struct {
    real32 r, g, b;
  };
  real32 e[2];
};

union v4 {
  struct {
    real32 x, y, z, w;
  };
  struct {
    real32 r, g, b, a;
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
AddRadiusWH(rectangle2 rect, real32 w, real32 h) {
  rectangle2 result = rect;
  result.min -= v2{w, h};
  result.max += v2{w, h};
  return result;
}

inline bool32
IsInRectangle(rectangle2 rect, v2 test) {
  bool32 result = (test.x >= rect.min.x) && (test.x <= rect.max.x) &&
                  (test.y >= rect.min.y) && (test.y <= rect.max.y);
  return result;
}

#endif
