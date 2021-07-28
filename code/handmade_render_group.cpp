inline v4
Unpack4x8(u32 color)
{
  v4 result = { (f32)((color >> 16) & 0xff),
    (f32)((color >> 8) & 0xff),
    (f32)((color >> 0) & 0xff),
    (f32)((color >> 24) & 0xff) };
  return result;
}

inline u32
Pack4x8(v4 value)
{
  u32 result = ((u32)(value.a + 0.5f) << 24) | ((u32)(value.r + 0.5f) << 16)
    | ((u32)(value.g + 0.5f) << 8) | (u32)(value.b + 0.5f);

  return result;
}

inline u32
SRGB1ToUint32(v4 color)
{
  u32 result = Pack4x8(color * 255.0f);

  return result;
}

inline v4
UnscaleAndBiasNormal(v4 normal)
{
  v4 result;

  f32 inv255 = 1.0f / 255.0f;

  result.x = -1.0f + 2.0f * (normal.x * inv255);
  result.y = -1.0f + 2.0f * (normal.y * inv255);
  result.z = -1.0f + 2.0f * (normal.z * inv255);

  result.w = normal.w * inv255;

  return result;
}

inline v4
SRGB255ToLinear1(v4 color)
{
  v4 result = {};
  f32 inv255 = 1.0f / 255.0f;
  result.r = Square(color.r * inv255);
  result.g = Square(color.g * inv255);
  result.b = Square(color.b * inv255);
  result.a = color.a * inv255;
  return result;
}

inline v4
Linear1ToSRGB255(v4 color)
{
  v4 result = {};
  result.r = 255.0f * SquareRoot(color.r);
  result.g = 255.0f * SquareRoot(color.g);
  result.b = 255.0f * SquareRoot(color.b);
  result.a = 255.0f * color.a;
  return result;
}

internal void
DrawBitmap(loaded_bitmap *buffer,
  loaded_bitmap *bitmap,
  v2 minCorner,
  f32 cAlpha = 1.0f)
{
  i32 minX = RoundReal32ToInt32(minCorner.x);
  i32 minY = RoundReal32ToInt32(minCorner.y);
  i32 maxX = minX + bitmap->width;
  i32 maxY = minY + bitmap->height;

  i32 clipX = 0;
  if(minX < 0) {
    clipX = -minX;
    minX = 0;
  }
  i32 clipY = 0;
  if(minY < 0) {
    clipY = -minY;
    minY = 0;
  }
  if(maxX > buffer->width) {
    maxX = buffer->width;
  }
  if(maxY > buffer->height) {
    maxY = buffer->height;
  }

  u8 *sourceRow
    = (u8 *)bitmap->memory + clipY * bitmap->pitch + clipX * BYTES_PER_PIXEL;
  u8 *destRow
    = (u8 *)buffer->memory + minY * buffer->pitch + minX * BYTES_PER_PIXEL;

  for(int y = minY; y < maxY; y++) {
    u32 *source = (u32 *)sourceRow;
    u32 *dest = (u32 *)destRow;

    for(int x = minX; x < maxX; x++) {
      v4 texel = {
        (f32)((*source >> 16) & 0xff),
        (f32)((*source >> 8) & 0xff),
        (f32)((*source >> 0) & 0xff),
        (f32)((*source >> 24) & 0xff),
      };

      texel = SRGB255ToLinear1(texel);

      texel *= cAlpha;

      v4 d = {
        (f32)((*dest >> 16) & 0xff),
        (f32)((*dest >> 8) & 0xff),
        (f32)((*dest >> 0) & 0xff),
        (f32)((*dest >> 24) & 0xff),
      };
      d = SRGB255ToLinear1(d);

      v4 blended = (1.0f - texel.a) * d + texel;
      blended = Linear1ToSRGB255(blended);

      // clang-format off
      *dest = ((u32)(blended.a + 0.5f) << 24) |
        ((u32)(blended.r + 0.5f) << 16) |
        ((u32)(blended.g + 0.5f) << 8) |
        ((u32)(blended.b + 0.5f));
      // clang-format on

      dest++;
      source++;
    }

    sourceRow += bitmap->pitch;
    destRow += buffer->pitch;
  }
}

internal void
DrawMatte(loaded_bitmap *buffer,
  loaded_bitmap *bitmap,
  v2 minCorner,
  f32 cAlpha = 1.0f)
{
  i32 minX = RoundReal32ToInt32(minCorner.x);
  i32 minY = RoundReal32ToInt32(minCorner.y);
  i32 maxX = minX + bitmap->width;
  i32 maxY = minY + bitmap->height;

  i32 clipX = 0;
  if(minX < 0) {
    clipX = -minX;
    minX = 0;
  }
  i32 clipY = 0;
  if(minY < 0) {
    clipY = -minY;
    minY = 0;
  }
  if(maxX > buffer->width) {
    maxX = buffer->width;
  }
  if(maxY > buffer->height) {
    maxY = buffer->height;
  }

  u8 *sourceRow
    = (u8 *)bitmap->memory + clipY * bitmap->pitch + clipX * BYTES_PER_PIXEL;
  u8 *destRow
    = (u8 *)buffer->memory + minY * buffer->pitch + minX * BYTES_PER_PIXEL;

  for(int y = minY; y < maxY; y++) {
    u32 *source = (u32 *)sourceRow;
    u32 *dest = (u32 *)destRow;

    for(int x = minX; x < maxX; x++) {
      v4 texel = {
        (f32)((*source >> 16) & 0xff),
        (f32)((*source >> 8) & 0xff),
        (f32)((*source >> 0) & 0xff),
        (f32)((*source >> 24) & 0xff),
      };

      texel = SRGB255ToLinear1(texel);
      texel *= cAlpha;

      v4 d = {
        (f32)((*dest >> 16) & 0xff),
        (f32)((*dest >> 8) & 0xff),
        (f32)((*dest >> 0) & 0xff),
        (f32)((*dest >> 24) & 0xff),
      };

      v4 blended = (1.0f - texel.a) * d;

      blended = Linear1ToSRGB255(blended);

      // clang-format off
      *dest = ((u32)(blended.a + 0.5f) << 24) |
        ((u32)(blended.r + 0.5f) << 16) |
        ((u32)(blended.g + 0.5f) << 8) |
        ((u32)(blended.b + 0.5f));
      // clang-format on

      dest++;
      source++;
    }

    sourceRow += bitmap->pitch;
    destRow += buffer->pitch;
  }
}

struct bilinear_sample {
  u32 a, b, c, d;
};

inline bilinear_sample
BilinearSample(loaded_bitmap *bitmap, i32 x, i32 y)
{
  Assert(x >= 0 && x < bitmap->width - 1);
  Assert(y >= 0 && y < bitmap->height - 1);

  bilinear_sample result;
  u8 *ptr = (u8 *)(bitmap->memory) + y * bitmap->pitch + x * 4;

  result.a = *((u32 *)ptr);
  result.b = *((u32 *)(ptr + 4));
  result.c = *((u32 *)(ptr + bitmap->pitch));
  result.d = *((u32 *)(ptr + bitmap->pitch + 4));

  return result;
}

v4
SRGBBilinearBlend(bilinear_sample sample, f32 fx, f32 fy)
{
  v4 texelA = Unpack4x8(sample.a);
  v4 texelB = Unpack4x8(sample.b);
  v4 texelC = Unpack4x8(sample.c);
  v4 texelD = Unpack4x8(sample.d);

  texelA = SRGB255ToLinear1(texelA);
  texelB = SRGB255ToLinear1(texelB);
  texelC = SRGB255ToLinear1(texelC);
  texelD = SRGB255ToLinear1(texelD);

  v4 result = Lerp(Lerp(texelA, fx, texelB), fy, Lerp(texelC, fx, texelD));

  return result;
}

inline v3
SampleEnvironmentMap(v2 screenSpaceUV,
  v3 sampleDirection,
  f32 roughness,
  environment_map *map,
  f32 distanceFromMapInZ)
{
  /*
    NOTE(cj):

    screenSpaceUV tells us where the ray is being cast _from_ in normalized
    screen coordinates.

    sampleDirection tells us what direction the cast is going - it does not have
    to be normalized.

    roughness says which LODs of map we sample from.

    distanceFromMapInZ says how far the map is from the sample point in Z, given
    in meters.
  */

  // NOTE(cj): Pick which LOD to sample from
  u32 lodIndex = (u32)(roughness * (f32)(ArrayCount(map->lod) - 1) + 0.5f);
  Assert(lodIndex < ArrayCount(map->lod));

  loaded_bitmap *lod = map->lod + lodIndex;

  f32 uvsPerMeter = 0.1f;
  f32 C = distanceFromMapInZ / sampleDirection.y;
  v2 offset = C * V2(sampleDirection.x, sampleDirection.z);
  v2 uv = screenSpaceUV + uvsPerMeter * offset;

  uv = Clamp01(uv);

  f32 tx = uv.x * (f32)(lod->width - 2);
  f32 ty = uv.y * (f32)(lod->height - 2);

  i32 x = (i32)tx;
  i32 y = (i32)ty;

  f32 fx = tx - (f32)x;
  f32 fy = ty - (f32)y;

  Assert(x >= 0 && x < lod->width - 1);
  Assert(y >= 0 && y < lod->height - 1);

#if 0
  u8 *texelPtr = (u8 *)lod->memory + y * lod->pitch + x * 4;
  *(u32 *)texelPtr = 0xFFFFFFFF;
#endif

  bilinear_sample sample = BilinearSample(lod, x, y);
  v3 result = SRGBBilinearBlend(sample, fx, fy).xyz;

  return result;
}

internal void
DrawRectangleHopefullyQuickly(loaded_bitmap *buffer,
  v2 origin,
  v2 xAxis,
  v2 yAxis,
  v4 color,
  loaded_bitmap *texture,
  f32 pixelsToMeters)
{
  BEGIN_TIMED_BLOCK(DrawRectangleHopefullyQuickly);

  __m128 a = _mm_set1_ps(1.0f);
  __m128 b = _mm_set1_ps(2.0f);
  __m128 result = _mm_add_ps(a, b);

  color.rgb *= color.a;

  f32 invXAxisLengthSq = 1.0f / LengthSq(xAxis);
  f32 invYAxisLengthSq = 1.0f / LengthSq(yAxis);

  i32 maxWidth = buffer->width - 1;
  i32 maxHeight = buffer->height - 1;

  f32 invMaxWidth = 1.0f / (f32)maxWidth;
  f32 invMaxHeight = 1.0f / (f32)maxHeight;

  i32 minX = maxWidth;
  i32 minY = maxHeight;
  i32 maxX = 0;
  i32 maxY = 0;

  f32 originZ = 0.0f;
  f32 originY = (origin + 0.5f * xAxis + 0.5f * yAxis).y;
  f32 fixedCastY = invMaxHeight * originY;

  v2 ps[4] = { origin, origin + xAxis, origin + yAxis, origin + xAxis + yAxis };
  for(int pIndex = 0; pIndex < ArrayCount(ps); pIndex++) {
    v2 p = ps[pIndex];
    i32 floorX = FloorReal32ToInt32(p.x);
    i32 floorY = FloorReal32ToInt32(p.y);

    if(minX > floorX) {
      minX = floorX;
    }
    if(maxX < floorX) {
      maxX = floorX;
    }
    if(minY > floorY) {
      minY = floorY;
    }
    if(maxY < floorY) {
      maxY = floorY;
    }
  }

  if(minX < 0) {
    minX = 0;
  }
  if(minY < 0) {
    minY = 0;
  }
  if(maxX > maxWidth) {
    maxX = maxWidth;
  }
  if(maxY > maxHeight) {
    maxY = maxHeight;
  }

  u8 *row = (u8 *)buffer->memory + minY * buffer->pitch + minX * 4;

  v2 nXAxis = xAxis * invXAxisLengthSq;
  v2 nYAxis = yAxis * invYAxisLengthSq;
  __m128 nXAxisx_4x = _mm_set1_ps(nXAxis.x);
  __m128 nXAxisy_4x = _mm_set1_ps(nXAxis.y);
  __m128 nYAxisx_4x = _mm_set1_ps(nYAxis.x);
  __m128 nYAxisy_4x = _mm_set1_ps(nYAxis.y);

  f32 inv255 = 1.0f / 255.0f;
  __m128 inv255_4x = _mm_set1_ps(inv255);
  __m128 one_4x = _mm_set1_ps(1.0f);
  __m128 zero_4x = _mm_set1_ps(0.0f);
  __m128 n255_4x = _mm_set1_ps(255.0f);

  __m128 colorr_4x = _mm_set1_ps(color.r);
  __m128 colorg_4x = _mm_set1_ps(color.g);
  __m128 colorb_4x = _mm_set1_ps(color.b);
  __m128 colora_4x = _mm_set1_ps(color.a);

  __m128 originx_4x = _mm_set1_ps(origin.x);
  __m128 originy_4x = _mm_set1_ps(origin.y);

#define M(a, idx) ((f32 *)(&a))[idx]
#define mmSquare(a) _mm_mul_ps(a, a)

  BEGIN_TIMED_BLOCK(ProcessPixel);
  for(int y = minY; y <= maxY; y++) {
    u32 *pixel = (u32 *)row;

    for(int x = minX; x <= maxX; x += 4) {
      __m128 texelAr = _mm_set1_ps(0.0f);
      __m128 texelAg = _mm_set1_ps(0.0f);
      __m128 texelAb = _mm_set1_ps(0.0f);
      __m128 texelAa = _mm_set1_ps(0.0f);

      __m128 texelBr = _mm_set1_ps(0.0f);
      __m128 texelBg = _mm_set1_ps(0.0f);
      __m128 texelBb = _mm_set1_ps(0.0f);
      __m128 texelBa = _mm_set1_ps(0.0f);

      __m128 texelCr = _mm_set1_ps(0.0f);
      __m128 texelCg = _mm_set1_ps(0.0f);
      __m128 texelCb = _mm_set1_ps(0.0f);
      __m128 texelCa = _mm_set1_ps(0.0f);

      __m128 texelDr = _mm_set1_ps(0.0f);
      __m128 texelDg = _mm_set1_ps(0.0f);
      __m128 texelDb = _mm_set1_ps(0.0f);
      __m128 texelDa = _mm_set1_ps(0.0f);

      __m128 fx = _mm_set1_ps(0.0f);
      __m128 fy = _mm_set1_ps(0.0f);

      __m128 destR = _mm_set1_ps(0.0f);
      __m128 destG = _mm_set1_ps(0.0f);
      __m128 destB = _mm_set1_ps(0.0f);
      __m128 destA = _mm_set1_ps(0.0f);

      __m128 blendedR, blendedG, blendedB, blendedA;
      bool32 shouldFill[4];

#if 1
      __m128 px
        = _mm_set_ps((f32)(x + 3), (f32)(x + 2), (f32)(x + 1), (f32)(x + 0));
      __m128 py = _mm_set1_ps((f32)y);
      __m128 dx = _mm_sub_ps(px, originx_4x);
      __m128 dy = _mm_sub_ps(py, originy_4x);
      __m128 u
        = _mm_add_ps(_mm_mul_ps(dx, nXAxisx_4x), _mm_mul_ps(dy, nXAxisy_4x));
      __m128 v
        = _mm_add_ps(_mm_mul_ps(dx, nYAxisx_4x), _mm_mul_ps(dy, nYAxisy_4x));
#endif

      for(int i = 0; i < 4; i++) {

#if 0
        v2 p = V2(x + i, y);
        v2 d = p - origin;
        f32 u = Inner(d, nXAxis);
        f32 v = Inner(d, nYAxis);
#endif
#if 0
        // !!! this code uses about 30 less cycles , from 140 tommkk 110
        // this is unbelievable
        f32 px = f32(x + i);
        f32 py = f32(y);
        f32 dx = px - origin.x;
        f32 dy = py - origin.y;
        f32 u = dx * nXAxis.x + dy * nXAxis.y;
        f32 v = dx * nYAxis.x + dy * nYAxis.y;
#endif

        shouldFill[i] = (M(u, i) >= 0.0f) && (M(u, i) < 1.0f)
          && (M(v, i) >= 0.0f) && (M(v, i) < 1.0f);
        // shouldFill[i] = (u >= 0.0f) && (u < 1.0f) && (v >= 0.0f) && (v
        // < 1.0f);

        if(shouldFill[i]) {
#if 1
          f32 tPx = M(u, i) * (f32)(texture->width - 2);
          f32 tPy = M(v, i) * (f32)(texture->height - 2);
#else
          f32 tPx = u * (f32)(texture->width - 2);
          f32 tPy = v * (f32)(texture->height - 2);
#endif

          i32 tx = (i32)tPx;
          i32 ty = (i32)tPy;

          M(fx, i) = tPx - (f32)tx;
          M(fy, i) = tPy - (f32)ty;

          u8 *ptr = (u8 *)(texture->memory) + ty * texture->pitch + tx * 4;
          u32 sampleA = *((u32 *)ptr);
          u32 sampleB = *((u32 *)(ptr + 4));
          u32 sampleC = *((u32 *)(ptr + texture->pitch));
          u32 sampleD = *((u32 *)(ptr + texture->pitch + 4));

          M(texelAr, i) = (f32)((sampleA >> 16) & 0xff);
          M(texelAg, i) = (f32)((sampleA >> 8) & 0xff);
          M(texelAb, i) = (f32)((sampleA >> 0) & 0xff);
          M(texelAa, i) = (f32)((sampleA >> 24) & 0xff);

          M(texelBr, i) = (f32)((sampleB >> 16) & 0xff);
          M(texelBg, i) = (f32)((sampleB >> 8) & 0xff);
          M(texelBb, i) = (f32)((sampleB >> 0) & 0xff);
          M(texelBa, i) = (f32)((sampleB >> 24) & 0xff);

          M(texelCr, i) = (f32)((sampleC >> 16) & 0xff);
          M(texelCg, i) = (f32)((sampleC >> 8) & 0xff);
          M(texelCb, i) = (f32)((sampleC >> 0) & 0xff);
          M(texelCa, i) = (f32)((sampleC >> 24) & 0xff);

          M(texelDr, i) = (f32)((sampleD >> 16) & 0xff);
          M(texelDg, i) = (f32)((sampleD >> 8) & 0xff);
          M(texelDb, i) = (f32)((sampleD >> 0) & 0xff);
          M(texelDa, i) = (f32)((sampleD >> 24) & 0xff);

          M(destR, i) = (f32)((*(pixel + i) >> 16) & 0xff);
          M(destG, i) = (f32)((*(pixel + i) >> 8) & 0xff);
          M(destB, i) = (f32)((*(pixel + i) >> 0) & 0xff);
          M(destA, i) = (f32)((*(pixel + i) >> 24) & 0xff);
        }
      }

      texelAr = mmSquare(_mm_mul_ps(texelAr, inv255_4x));
      texelAg = mmSquare(_mm_mul_ps(texelAg, inv255_4x));
      texelAb = mmSquare(_mm_mul_ps(texelAb, inv255_4x));
      texelAa = _mm_mul_ps(texelAa, inv255_4x);

      texelBr = mmSquare(_mm_mul_ps(texelBr, inv255_4x));
      texelBg = mmSquare(_mm_mul_ps(texelBg, inv255_4x));
      texelBb = mmSquare(_mm_mul_ps(texelBb, inv255_4x));
      texelBa = _mm_mul_ps(texelBa, inv255_4x);

      texelCr = mmSquare(_mm_mul_ps(texelCr, inv255_4x));
      texelCg = mmSquare(_mm_mul_ps(texelCg, inv255_4x));
      texelCb = mmSquare(_mm_mul_ps(texelCb, inv255_4x));
      texelCa = _mm_mul_ps(texelCa, inv255_4x);

      texelDr = mmSquare(_mm_mul_ps(texelDr, inv255_4x));
      texelDg = mmSquare(_mm_mul_ps(texelDg, inv255_4x));
      texelDb = mmSquare(_mm_mul_ps(texelDb, inv255_4x));
      texelDa = _mm_mul_ps(texelDa, inv255_4x);

      __m128 invFx = _mm_sub_ps(one_4x, fx);
      __m128 invFy = _mm_sub_ps(one_4x, fy);
      __m128 l0 = _mm_mul_ps(invFy, invFx);
      __m128 l1 = _mm_mul_ps(invFy, fx);
      __m128 l2 = _mm_mul_ps(fy, invFx);
      __m128 l3 = _mm_mul_ps(fy, fx);

      __m128 texelR = _mm_add_ps(_mm_mul_ps(l0, texelAr),
        _mm_add_ps(_mm_mul_ps(l1, texelBr),
          _mm_add_ps(_mm_mul_ps(l2, texelCr), _mm_mul_ps(l3, texelDr))));

      __m128 texelG = _mm_add_ps(_mm_mul_ps(l0, texelAg),
        _mm_add_ps(_mm_mul_ps(l1, texelBg),
          _mm_add_ps(_mm_mul_ps(l2, texelCg), _mm_mul_ps(l3, texelDg))));

      __m128 texelB = _mm_add_ps(_mm_mul_ps(l0, texelAb),
        _mm_add_ps(_mm_mul_ps(l1, texelBb),
          _mm_add_ps(_mm_mul_ps(l2, texelCb), _mm_mul_ps(l3, texelDb))));

      __m128 texelA = _mm_add_ps(_mm_mul_ps(l0, texelAa),
        _mm_add_ps(_mm_mul_ps(l1, texelBa),
          _mm_add_ps(_mm_mul_ps(l2, texelCa), _mm_mul_ps(l3, texelDa))));

      texelR = _mm_mul_ps(texelR, colorr_4x);
      texelG = _mm_mul_ps(texelG, colorg_4x);
      texelB = _mm_mul_ps(texelB, colorb_4x);
      texelA = _mm_mul_ps(texelA, colora_4x);

      texelR = _mm_min_ps(_mm_max_ps(texelR, zero_4x), one_4x);
      texelG = _mm_min_ps(_mm_max_ps(texelG, zero_4x), one_4x);
      texelB = _mm_min_ps(_mm_max_ps(texelB, zero_4x), one_4x);

      __m128 invTexelA = _mm_sub_ps(one_4x, texelA);

      destR = mmSquare(_mm_mul_ps(destR, inv255_4x));
      destG = mmSquare(_mm_mul_ps(destG, inv255_4x));
      destB = mmSquare(_mm_mul_ps(destB, inv255_4x));
      destA = _mm_mul_ps(destA, inv255_4x);

      blendedR = _mm_add_ps(_mm_mul_ps(invTexelA, destR), texelR);
      blendedG = _mm_add_ps(_mm_mul_ps(invTexelA, destG), texelG);
      blendedB = _mm_add_ps(_mm_mul_ps(invTexelA, destB), texelB);
      blendedA = _mm_add_ps(_mm_mul_ps(invTexelA, destA), texelA);

      blendedR = _mm_mul_ps(n255_4x, _mm_sqrt_ps(blendedR));
      blendedG = _mm_mul_ps(n255_4x, _mm_sqrt_ps(blendedG));
      blendedB = _mm_mul_ps(n255_4x, _mm_sqrt_ps(blendedB));
      blendedA = _mm_mul_ps(n255_4x, blendedA);

      for(int i = 0; i < 4; i++) {
        if(shouldFill[i]) {
          *(pixel + i) = ((u32)(M(blendedA, i) + 0.5f) << 24)
            | ((u32)(M(blendedR, i) + 0.5f) << 16)
            | ((u32)(M(blendedG, i) + 0.5f) << 8)
            | (u32)(M(blendedB, i) + 0.5f);
        }
      }

      pixel += 4;
    }

    row += buffer->pitch;
  }

  END_TIMED_BLOCK_COUNTED(ProcessPixel, (maxY - minY + 1) * (maxX - minX + 1));

  END_TIMED_BLOCK(DrawRectangleHopefullyQuickly);
}

// exclusive
internal void
DrawRectangleSlowly(loaded_bitmap *buffer,
  v2 origin,
  v2 xAxis,
  v2 yAxis,
  v4 color,
  loaded_bitmap *texture,
  loaded_bitmap *normalMap,
  environment_map *top,
  environment_map *middle,
  environment_map *bottom,
  f32 pixelsToMeters)
{
  BEGIN_TIMED_BLOCK(DrawRectangleSlowly);

  color.rgb *= color.a;

  u32 c = SRGB1ToUint32(color);

  f32 xAxisLength = Length(xAxis);
  f32 yAxisLength = Length(yAxis);

  v2 nXAxis = (yAxisLength / xAxisLength) * xAxis;
  v2 nYAxis = (xAxisLength / yAxisLength) * yAxis;

  // NOTE(cj): this could be a parameter if we want people to
  // have control over the amount of scaling in the Z direction.
  f32 nZScale = 0.5f * (xAxisLength + yAxisLength);

  f32 invXAxisLengthSq = 1.0f / LengthSq(xAxis);
  f32 invYAxisLengthSq = 1.0f / LengthSq(yAxis);

  i32 maxWidth = buffer->width - 1;
  i32 maxHeight = buffer->height - 1;

  f32 invMaxWidth = 1.0f / (f32)maxWidth;
  f32 invMaxHeight = 1.0f / (f32)maxHeight;

  i32 minX = maxWidth;
  i32 minY = maxHeight;
  i32 maxX = 0;
  i32 maxY = 0;

  f32 originZ = 0.0f;
  f32 originY = (origin + 0.5f * xAxis + 0.5f * yAxis).y;
  f32 fixedCastY = invMaxHeight * originY;

  v2 ps[4] = { origin, origin + xAxis, origin + yAxis, origin + xAxis + yAxis };
  for(int pIndex = 0; pIndex < ArrayCount(ps); pIndex++) {
    v2 p = ps[pIndex];
    i32 floorX = FloorReal32ToInt32(p.x);
    i32 floorY = FloorReal32ToInt32(p.y);

    if(minX > floorX) {
      minX = floorX;
    }
    if(maxX < floorX) {
      maxX = floorX;
    }
    if(minY > floorY) {
      minY = floorY;
    }
    if(maxY < floorY) {
      maxY = floorY;
    }
  }

  if(minX < 0) {
    minX = 0;
  }
  if(minY < 0) {
    minY = 0;
  }
  if(maxX > maxWidth) {
    maxX = maxWidth;
  }
  if(maxY > maxHeight) {
    maxY = maxHeight;
  }

  u8 *row = (u8 *)buffer->memory + minY * buffer->pitch + minX * 4;

  for(int y = minY; y <= maxY; y++) {
    u32 *pixel = (u32 *)row;

    for(int x = minX; x <= maxX; x++) {
      v2 p = V2(x, y);
      v2 d = p - origin;

      f32 edgeTop = Inner(Perp(xAxis), d - yAxis);
      f32 edgeBottom = Inner(-Perp(xAxis), d);
      f32 edgeLeft = Inner(Perp(yAxis), d);
      f32 edgeRight = Inner(-Perp(yAxis), d - xAxis);

      if((edgeTop < 0) && (edgeBottom < 0) && (edgeLeft < 0)
        && (edgeRight < 0)) {
        v2 screenSpaceUV = { invMaxWidth * (f32)x, fixedCastY };
        f32 zDiff = pixelsToMeters * ((f32)y - originY);

        // NOTE(cj): xAxis must be perpendicular with yAxis
        f32 u = Inner(d, xAxis) * invXAxisLengthSq;
        f32 v = Inner(d, yAxis) * invYAxisLengthSq;

        // f32 epsilon = 1.0f;
        Assert(u >= 0.0f && u <= 1.0f);
        Assert(v >= 0.0f && v <= 1.0f);

        // TODO: SSE clamping
        f32 tPx = u * (f32)(texture->width - 2);
        f32 tPy = v * (f32)(texture->height - 2);

        i32 tx = (i32)tPx;
        i32 ty = (i32)tPy;

        f32 fx = tPx - (f32)tx;
        f32 fy = tPy - (f32)ty;

        bilinear_sample bilinearSample = BilinearSample(texture, tx, ty);
        v4 texel = SRGBBilinearBlend(bilinearSample, fx, fy);

        if(normalMap) {
          bilinear_sample normalSample = BilinearSample(normalMap, tx, ty);
          v4 normalA = Unpack4x8(normalSample.a);
          v4 normalB = Unpack4x8(normalSample.b);
          v4 normalC = Unpack4x8(normalSample.c);
          v4 normalD = Unpack4x8(normalSample.d);

          v4 normal
            = Lerp(Lerp(normalA, fx, normalB), fy, Lerp(normalC, fx, normalD));

          normal = UnscaleAndBiasNormal(normal);

          // Rotate the normal
          normal.xy = normal.x * nXAxis + normal.y * nYAxis;
          normal.z *= nZScale;
          normal.xyz = Normalize(normal.xyz);

          // NOTE(cj): The eye vector is always assumed to be [0, 0, 1]
          // This is the simplified version of -e + 2Inner(e, N)N
          v3 bouceDirection = 2.0f * normal.z * normal.xyz - V3(0, 0, 1);
          bouceDirection.z = -bouceDirection.z;

          environment_map *farMap = 0;
          f32 z = originZ + zDiff;
          f32 mapZ = 2.0f;
          f32 tEnvMap = bouceDirection.y;
          f32 tFarMap = 0.0f;

          if(tEnvMap < -0.5f) {
            farMap = bottom;
            tFarMap = 2.0f * (-tEnvMap - 0.5f);
          } else if(tEnvMap > 0.5f) {
            farMap = top;
            tFarMap = 2.0f * (tEnvMap - 0.5f);
          }

          tFarMap *= tFarMap;
          tFarMap *= tFarMap;

          v3 lightColor = { 0, 0, 0 };
          if(farMap) {
            f32 distanceFromMapInZ = farMap->z - z;
            v3 farMapColor = SampleEnvironmentMap(screenSpaceUV,
              bouceDirection,
              normal.w,
              farMap,
              distanceFromMapInZ);
            lightColor = Lerp(lightColor, tFarMap, farMapColor);
          }

          texel.rgb = texel.rgb + texel.a * lightColor;
          // texel.rgb = V3(0.5f, 0.5f, 0.5f) + 0.5f * bouceDirection;
        }

        texel = Hadamard(texel, color);
        texel.r = Clamp01(texel.r);
        texel.g = Clamp01(texel.g);
        texel.b = Clamp01(texel.b);

        v4 dest = SRGB255ToLinear1(Unpack4x8(*pixel));

        // texel is premultiplied alpha
        v4 blended = (1.0f - texel.a) * dest + texel;

        blended = Linear1ToSRGB255(blended);
        *pixel = Pack4x8(blended);
      }

      pixel++;
    }

    row += buffer->pitch;
  }

  END_TIMED_BLOCK(DrawRectangleSlowly);
}

// exclusive: [min, max)
internal void
DrawRectangle(loaded_bitmap *buffer, v2 min, v2 max, v4 color)
{
  i32 minX = RoundReal32ToInt32(min.x);
  i32 minY = RoundReal32ToInt32(min.y);
  i32 maxX = RoundReal32ToInt32(max.x);
  i32 maxY = RoundReal32ToInt32(max.y);

  if(minX < 0) {
    minX = 0;
  }
  if(minY < 0) {
    minY = 0;
  }
  if(maxX > buffer->width) {
    maxX = buffer->width;
  }
  if(maxY > buffer->height) {
    maxY = buffer->height;
  }

  u32 c = SRGB1ToUint32(color);

  u8 *row
    = (u8 *)buffer->memory + minY * buffer->pitch + minX * BYTES_PER_PIXEL;

  for(int y = minY; y < maxY; y++) {
    u32 *Pixel = (u32 *)row;
    for(int x = minX; x < maxX; x++) {
      *Pixel++ = c;
    }

    row += buffer->pitch;
  }
}

internal void
DrawRectangleOutline(loaded_bitmap *buffer, v2 min, v2 max, v4 color)
{
  f32 r = 2;

  // top and bottom
  DrawRectangle(buffer,
    V2(min.x - r, max.y - r),
    V2(max.x + r, max.y + r),
    color);
  DrawRectangle(buffer,
    V2(min.x - r, min.y - r),
    V2(max.x + r, min.y + r),
    color);

  // left and right
  DrawRectangle(buffer,
    V2(min.x - r, min.y - r),
    V2(min.x + r, max.y + r),
    color);
  DrawRectangle(buffer,
    V2(max.x - r, min.y - r),
    V2(max.x + r, max.y + r),
    color);
}

internal render_group *
AllocateRenderGroup(memory_arena *arena,
  u32 maxPushBufferSize,
  u32 resolutionPixelsX,
  u32 resolutionPixelsY)
{
  render_group *result = PushStruct(arena, render_group);
  render_basis *defaultBasis = PushStruct(arena, render_basis);
  defaultBasis->p = {};

  result->defaultBasis = defaultBasis;
  result->pieceCount = 0;

  result->maxPushBufferSize = maxPushBufferSize;
  result->pushBufferSize = 0;
  result->pushBufferBase = (u8 *)PushSize(arena, maxPushBufferSize);

  f32 widthOfMonitor = 0.635f;

  result->metersToPixels = resolutionPixelsX / widthOfMonitor;
  result->gameCamera.focalLength = 0.6f;
  result->gameCamera.cameraDistanceAboveTarget = 25.0f;

  result->renderCamera = result->gameCamera;
  // result->renderCamera.cameraDistanceAboveTarget = 60.0f;

  f32 pixelsToMeters = 1.0f / result->metersToPixels;

  result->monitorHalfDim = 0.5f
    * V2(resolutionPixelsX * pixelsToMeters,
      resolutionPixelsY * pixelsToMeters);

  result->globalAlpha = 1.0f;

  return result;
}

#define PushRenderElement(group, type)                                         \
  (type *)_PushRenderElement(group, sizeof(type), RenderEntryType_##type)

internal void *
_PushRenderElement(render_group *group, u32 size, render_entry_type type)
{
  size += sizeof(render_entry_header);
  Assert(group->pushBufferSize + size <= group->maxPushBufferSize);

  render_entry_header *header
    = (render_entry_header *)(group->pushBufferBase + group->pushBufferSize);
  group->pushBufferSize += size;

  header->type = type;

  return header + 1;
}

// `offset` is from the min corner
internal void
PushBitmap(render_group *group,
  loaded_bitmap *bitmap,
  f32 height,
  v3 offset,
  v4 color = V4(1, 1, 1, 1))
{
  render_entry_bitmap *entry = PushRenderElement(group, render_entry_bitmap);
  v2 size = V2(height * bitmap->widthOverHeight, height);
  v2 align = Hadamard(bitmap->alignPercentage, size);
  entry->entityBasis.offset = offset - V3(align, 0.0f);
  entry->entityBasis.basis = group->defaultBasis;
  entry->bitmap = bitmap;
  entry->color = group->globalAlpha * color;
  entry->size = size;
}

internal void
Clear(render_group *group, v4 color)
{
  render_entry_clear *entry = PushRenderElement(group, render_entry_clear);
  entry->color = color;
}

internal render_entry_coordinate_system *
CoordinateSystem(render_group *group,
  v2 origin,
  v2 xAxis,
  v2 yAxis,
  v4 color,
  loaded_bitmap *texture,
  loaded_bitmap *normalMap,
  environment_map *top,
  environment_map *middle,
  environment_map *bottom)
{
  render_entry_coordinate_system *entry
    = PushRenderElement(group, render_entry_coordinate_system);

  entry->origin = origin;
  entry->xAxis = xAxis;
  entry->yAxis = yAxis;
  entry->color = color;
  entry->texture = texture;
  entry->normalMap = normalMap;
  entry->top = top;
  entry->middle = middle;
  entry->bottom = bottom;

  return entry;
}

// `offset` is from the center
internal void
PushRect(render_group *group, v3 offset, v2 dim, v4 color = V4(1, 1, 1, 1))
{
  render_entry_rectangle *entry
    = PushRenderElement(group, render_entry_rectangle);
  entry->entityBasis.offset = offset - V3(0.5f * dim, 0.0f);
  entry->entityBasis.basis = group->defaultBasis;
  entry->color = color;
  entry->dim = dim;
}

// `offset` is from the center
internal void
PushRectOutline(render_group *group,
  v3 offset,
  v2 dim,
  v4 color = V4(1, 1, 1, 1))
{
  f32 thickness = 0.1f;
  // Top and bottom
  PushRect(group,
    offset - V3(0.0f, 0.5f * dim.y, 0.0f),
    V2(dim.x, thickness),
    color);
  PushRect(group,
    offset + V3(0.0f, 0.5f * dim.y, 0.0f),
    V2(dim.x, thickness),
    color);

  // Left and right
  PushRect(group,
    offset - V3(0.5f * dim.x, 0.0f, 0.0f),
    V2(thickness, dim.y),
    color);
  PushRect(group,
    offset + V3(0.5f * dim.x, 0.0f, 0.0f),
    V2(thickness, dim.y),
    color);
}

struct render_basis_result {
  f32 scale;
  v2 p;
  bool32 valid;
};

internal render_basis_result
GetEntityRenderBasisResult(render_group *renderGroup,
  render_entity_basis *entityBasis,
  v2 screenDim,
  f32 metersToPixels)
{
  v2 screenCenter = 0.5f * screenDim;
  render_basis_result result = {};
  v3 entityP = entityBasis->basis->p;
  f32 zDistance
    = renderGroup->renderCamera.cameraDistanceAboveTarget - entityP.z;
  f32 nearClipPlane = 0.2f;

  v2 rawXY = entityP.xy + entityBasis->offset.xy;
  if(zDistance > nearClipPlane) {
    f32 scale = renderGroup->metersToPixels * (1.0f / zDistance)
      * renderGroup->renderCamera.focalLength;
    result.p = screenCenter + scale * rawXY;
    result.scale = scale;
    result.valid = true;
  }

  return result;
}

internal void
RenderGroupToOutput(render_group *renderGroup, loaded_bitmap *outputTarget)
{
  BEGIN_TIMED_BLOCK(Render);

  v2 screenDim = { (f32)outputTarget->width, (f32)outputTarget->height };

  f32 metersToPixels = screenDim.x / 20.0f;
  f32 pixelsToMeters = 1.0f / metersToPixels;
  v2 screenCenter = 0.5f * screenDim;

  for(u32 offset = 0; offset < renderGroup->pushBufferSize;) {
    render_entry_header *header
      = (render_entry_header *)(renderGroup->pushBufferBase + offset);
    offset += sizeof(render_entry_header);
    void *data = (void *)(header + 1);

    switch(header->type) {
      case RenderEntryType_render_entry_clear: {
        render_entry_clear *entry = (render_entry_clear *)data;
        offset += sizeof(render_entry_clear);

        DrawRectangle(outputTarget,
          V2(0, 0),
          V2((f32)outputTarget->width, (f32)outputTarget->height),
          entry->color);
      } break;

      case RenderEntryType_render_entry_rectangle: {
        render_entry_rectangle *entry = (render_entry_rectangle *)data;
        offset += sizeof(render_entry_rectangle);
        v3 entityP = entry->entityBasis.basis->p;

        render_basis_result basisResult
          = GetEntityRenderBasisResult(renderGroup,
            &entry->entityBasis,
            screenDim,
            metersToPixels);

        DrawRectangle(outputTarget,
          basisResult.p,
          basisResult.p + basisResult.scale * entry->dim,
          entry->color);
      } break;

      case RenderEntryType_render_entry_bitmap: {
        render_entry_bitmap *entry = (render_entry_bitmap *)data;
        offset += sizeof(render_entry_bitmap);

        render_basis_result basisResult
          = GetEntityRenderBasisResult(renderGroup,
            &entry->entityBasis,
            screenDim,
            metersToPixels);

        // DrawBitmap(outputTarget, entry->bitmap, min, entry->color.a);
        DrawRectangleHopefullyQuickly(outputTarget,
          basisResult.p,
          basisResult.scale * V2(entry->size.x, 0.0f),
          basisResult.scale * V2(0.0f, entry->size.y),
          entry->color,
          entry->bitmap,
          pixelsToMeters);

      } break;

      case RenderEntryType_render_entry_coordinate_system: {
        render_entry_coordinate_system *entry
          = (render_entry_coordinate_system *)data;
        offset += sizeof(render_entry_coordinate_system);

        v2 min = entry->origin;
        v2 max = entry->origin + entry->xAxis + entry->yAxis;
        DrawRectangleHopefullyQuickly(outputTarget,
          entry->origin,
          entry->xAxis,
          entry->yAxis,
          entry->color,
          entry->texture,
          pixelsToMeters);

        v2 dim = V2(4, 4);
        v2 p = entry->origin;
        v4 color = { 1.0f, 1.0f, 0.0f, 1.0f };
        DrawRectangle(outputTarget, p, p + dim, color);

        p = entry->origin + entry->xAxis;
        DrawRectangle(outputTarget, p, p + dim, color);

        p = entry->origin + entry->yAxis;
        DrawRectangle(outputTarget, p, p + dim, color);

        DrawRectangle(outputTarget, max, max + dim, color);
#if 0
        for(u32 i = 0; i < ArrayCount(entry->points); i++) {
          v2 p = entry->points[i];
          v2 min = entry->origin + p.x * entry->xAxis + p.y * entry->yAxis;
          DrawRectangle(outputTarget, min, min + dim, entry->color);
        }
#endif
      } break;

        InvalidDefaultCase;
    }
  }

  END_TIMED_BLOCK(Render);
}

internal v2
Unproject(v2 projected, f32 focalLength, f32 distanceToCamera)
{
  v2 result = projected * (distanceToCamera / focalLength);
  return result;
}

internal rectangle2
GetCameraRectangleAtDistance(render_group *renderGroup, f32 distanceToCamera)
{
  v2 halfDim = Unproject(renderGroup->monitorHalfDim,
    renderGroup->gameCamera.focalLength,
    distanceToCamera);
  rectangle2 result = RectCenterHalfDim(V2(0, 0), halfDim);
  return result;
}

internal rectangle2
GetCameraRectangleAtTarget(render_group *renderGroup)
{
  rectangle2 result = GetCameraRectangleAtDistance(renderGroup,
    renderGroup->gameCamera.cameraDistanceAboveTarget);
  return result;
}
