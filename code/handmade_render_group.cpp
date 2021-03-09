internal void
DrawBitmap(loaded_bitmap *buffer,
  loaded_bitmap *bitmap,
  v2 minCorner,
  real32 cAlpha = 1.0f)
{
  int32 minX = RoundReal32ToInt32(minCorner.x);
  int32 minY = RoundReal32ToInt32(minCorner.y);
  int32 maxX = minX + bitmap->width;
  int32 maxY = minY + bitmap->height;

  int32 clipX = 0;
  if(minX < 0) {
    clipX = -minX;
    minX = 0;
  }
  int32 clipY = 0;
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

  uint8 *sourceRow
    = (uint8 *)bitmap->memory + clipY * bitmap->pitch + clipX * BYTES_PER_PIXEL;
  uint8 *destRow
    = (uint8 *)buffer->memory + minY * buffer->pitch + minX * BYTES_PER_PIXEL;

  for(int y = minY; y < maxY; y++) {
    uint32 *source = (uint32 *)sourceRow;
    uint32 *dest = (uint32 *)destRow;

    for(int x = minX; x < maxX; x++) {
      real32 sA = (real32)((*source >> 24) & 0xff);
      real32 sR = cAlpha * (real32)((*source >> 16) & 0xff);
      real32 sG = cAlpha * (real32)((*source >> 8) & 0xff);
      real32 sB = cAlpha * (real32)((*source >> 0) & 0xff);

      real32 rSA = sA / 255.0f * cAlpha;

      real32 dA = (real32)((*dest >> 24) & 0xff);
      real32 dR = (real32)((*dest >> 16) & 0xff);
      real32 dG = (real32)((*dest >> 8) & 0xff);
      real32 dB = (real32)((*dest >> 0) & 0xff);

      real32 rDA = dA / 255.0f;

      real32 a = 255.0f * (rSA + rDA - rSA * rDA);
      real32 r = sR + (1 - rSA) * dR;
      real32 g = sG + (1 - rSA) * dG;
      real32 b = sB + (1 - rSA) * dB;

      // clang-format off
      *dest = ((uint32)(a + 0.5f) << 24) |
        ((uint32)(r + 0.5f) << 16) |
        ((uint32)(g + 0.5f) << 8) |
        ((uint32)(b + 0.5f));
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
  real32 cAlpha = 1.0f)
{
  int32 minX = RoundReal32ToInt32(minCorner.x);
  int32 minY = RoundReal32ToInt32(minCorner.y);
  int32 maxX = minX + bitmap->width;
  int32 maxY = minY + bitmap->height;

  int32 clipX = 0;
  if(minX < 0) {
    clipX = -minX;
    minX = 0;
  }
  int32 clipY = 0;
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

  uint8 *sourceRow
    = (uint8 *)bitmap->memory + clipY * bitmap->pitch + clipX * BYTES_PER_PIXEL;
  uint8 *destRow
    = (uint8 *)buffer->memory + minY * buffer->pitch + minX * BYTES_PER_PIXEL;

  for(int y = minY; y < maxY; y++) {
    uint32 *source = (uint32 *)sourceRow;
    uint32 *dest = (uint32 *)destRow;

    for(int x = minX; x < maxX; x++) {
      real32 sA = (real32)((*source >> 24) & 0xff);
      real32 sR = cAlpha * (real32)((*source >> 16) & 0xff);
      real32 sG = cAlpha * (real32)((*source >> 8) & 0xff);
      real32 sB = cAlpha * (real32)((*source >> 0) & 0xff);

      real32 rSA = sA / 255.0f * cAlpha;

      real32 dA = (real32)((*dest >> 24) & 0xff);
      real32 dR = (real32)((*dest >> 16) & 0xff);
      real32 dG = (real32)((*dest >> 8) & 0xff);
      real32 dB = (real32)((*dest >> 0) & 0xff);

      real32 rDA = dA / 255.0f;

      real32 invRSA = 1 - rSA;

      real32 a = invRSA * dA;
      real32 r = invRSA * dR;
      real32 g = invRSA * dG;
      real32 b = invRSA * dB;

      // clang-format off
      *dest = ((uint32)(a + 0.5f) << 24) |
        ((uint32)(r + 0.5f) << 16) |
        ((uint32)(g + 0.5f) << 8) |
        ((uint32)(b + 0.5f));
      // clang-format on

      dest++;
      source++;
    }

    sourceRow += bitmap->pitch;
    destRow += buffer->pitch;
  }
}

inline v4
Uint32ToSRGB255(uint32 color)
{
  v4 result = { (real32)((color >> 16) & 0xff),
    (real32)((color >> 8) & 0xff),
    (real32)((color >> 0) & 0xff),
    (real32)((color >> 24) & 0xff) };
  return result;
}

inline v4
SRGB255ToLinear1(v4 color)
{
  v4 result = {};
  real32 inv255 = 1.0f / 255.0f;
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

// exclusive
internal void
DrawRectangleSlowly(loaded_bitmap *buffer,
  v2 origin,
  v2 xAxis,
  v2 yAxis,
  v4 color,
  loaded_bitmap *texture)
{
  uint32 c = (RoundReal32ToUint32(color.r * 255.0f) << 16)
    | (RoundReal32ToUint32(color.g * 255.0f) << 8)
    | RoundReal32ToUint32(color.b * 255.0f);

  real32 invXAxisLengthSq = 1.0f / LengthSq(xAxis);
  real32 invYAxisLengthSq = 1.0f / LengthSq(yAxis);

  int32 maxWidth = buffer->width - 1;
  int32 maxHeight = buffer->height - 1;

  int32 minX = maxWidth;
  int32 minY = maxHeight;
  int32 maxX = 0;
  int32 maxY = 0;

  v2 ps[4] = { origin, origin + xAxis, origin + yAxis, origin + xAxis + yAxis };
  for(int pIndex = 0; pIndex < ArrayCount(ps); pIndex++) {
    v2 p = ps[pIndex];
    int32 floorX = FloorReal32ToInt32(p.x);
    int32 floorY = FloorReal32ToInt32(p.y);

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

  uint8 *row = (uint8 *)buffer->memory + minY * buffer->pitch + minX * 4;

  for(int y = minY; y <= maxY; y++) {
    uint32 *pixel = (uint32 *)row;

    for(int x = minX; x <= maxX; x++) {
      v2 p = V2(x, y);
      v2 d = p - origin;

      real32 edgeTop = Inner(Perp(xAxis), d - yAxis);
      real32 edgeBottom = Inner(-Perp(xAxis), d);
      real32 edgeLeft = Inner(Perp(yAxis), d);
      real32 edgeRight = Inner(-Perp(yAxis), d - xAxis);

      if((edgeTop < 0) && (edgeBottom < 0) && (edgeLeft < 0)
        && (edgeRight < 0)) {
        real32 u = Inner(d, xAxis) * invXAxisLengthSq;
        real32 v = Inner(d, yAxis) * invYAxisLengthSq;

        // real32 epsilon = 1.0f;
        Assert(u >= 0.0f && u <= 1.0f);
        Assert(v >= 0.0f && v <= 1.0f);

        // TODO: SSE clamping
        real32 tPx = u * (real32)(texture->width - 2);
        real32 tPy = v * (real32)(texture->height - 2);

        int32 tx = (int32)tPx;
        int32 ty = (int32)tPy;

        real32 fx = tPx - (real32)tx;
        real32 fy = tPy - (real32)ty;

        Assert(tx >= 0 && tx < texture->width - 1);
        Assert(ty >= 0 && ty < texture->height - 1);

        uint8 *texelPtr
          = (uint8 *)(texture->memory) + ty * texture->pitch + tx * 4;

        uint32 texelAValue = *((uint32 *)texelPtr);
        uint32 texelBValue = *((uint32 *)(texelPtr + 4));
        uint32 texelCValue = *((uint32 *)(texelPtr + texture->pitch));
        uint32 texelDValue = *((uint32 *)(texelPtr + texture->pitch + 4));

        v4 texelA = Uint32ToSRGB255(texelAValue);
        texelA = SRGB255ToLinear1(texelA);

        v4 texelB = Uint32ToSRGB255(texelBValue);
        texelB = SRGB255ToLinear1(texelB);

        v4 texelC = Uint32ToSRGB255(texelCValue);
        texelC = SRGB255ToLinear1(texelC);

        v4 texelD = Uint32ToSRGB255(texelDValue);
        texelD = SRGB255ToLinear1(texelD);

#if 1
        // Bilinear texture filtering
        v4 texel = Lerp(Lerp(texelA, fx, texelB), fy, Lerp(texelC, fx, texelD));
#else
        v4 texel = texelA;
#endif

        v4 dest = Uint32ToSRGB255(*pixel);
        dest = SRGB255ToLinear1(dest);

        real32 sA = texel.a;
        real32 sR = texel.r;
        real32 sG = texel.g;
        real32 sB = texel.b;
        real32 rSA = color.a * sA;

        real32 dA = dest.a;
        real32 dR = dest.r;
        real32 dG = dest.g;
        real32 dB = dest.b;
        real32 rDA = dest.a;

        v4 blended = {
          color.a * color.r * sR + (1 - rSA) * dR,
          color.a * color.g * sG + (1 - rSA) * dG,
          color.a * color.b * sB + (1 - rSA) * dB,
          rSA + rDA - rSA * rDA,
        };

        blended = Linear1ToSRGB255(blended);

        // clang-format off
      *pixel = ((uint32)(blended.a + 0.5f) << 24) |
        ((uint32)(blended.r + 0.5f) << 16) |
        ((uint32)(blended.g + 0.5f) << 8) |
        ((uint32)(blended.b + 0.5f));
        // clang-format on
      }

      pixel++;
    }

    row += buffer->pitch;
  }
}

// exclusive
internal void
DrawRectangle(loaded_bitmap *buffer, v2 min, v2 max, v4 color)
{
  int32 minX = RoundReal32ToInt32(min.x);
  int32 minY = RoundReal32ToInt32(min.y);
  int32 maxX = RoundReal32ToInt32(max.x);
  int32 maxY = RoundReal32ToInt32(max.y);

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

  uint32 c = (RoundReal32ToUint32(color.r * 255.0f) << 16)
    | (RoundReal32ToUint32(color.g * 255.0f) << 8)
    | RoundReal32ToUint32(color.b * 255.0f);

  uint8 *row
    = (uint8 *)buffer->memory + minY * buffer->pitch + minX * BYTES_PER_PIXEL;

  for(int y = minY; y < maxY; y++) {
    uint32 *Pixel = (uint32 *)row;
    for(int x = minX; x < maxX; x++) {
      *Pixel++ = c;
    }

    row += buffer->pitch;
  }
}

internal void
DrawRectangleOutline(loaded_bitmap *buffer, v2 min, v2 max, v4 color)
{
  real32 r = 2;

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
  uint32 maxPushBufferSize,
  real32 metersToPixels)
{
  render_group *result = PushStruct(arena, render_group);
  render_basis *defaultBasis = PushStruct(arena, render_basis);
  defaultBasis->p = {};

  result->defaultBasis = defaultBasis;
  result->pieceCount = 0;
  result->metersToPixels = metersToPixels;

  result->maxPushBufferSize = maxPushBufferSize;
  result->pushBufferSize = 0;
  result->pushBufferBase = (uint8 *)PushSize(arena, maxPushBufferSize);

  return result;
}

#define PushRenderElement(group, type)                                         \
  (type *)_PushRenderElement(group, sizeof(type), RenderEntryType_##type)

internal void *
_PushRenderElement(render_group *group, uint32 size, render_entry_type type)
{
  Assert(group->pushBufferSize + size <= group->maxPushBufferSize);

  render_entry_header *header
    = (render_entry_header *)(group->pushBufferBase + group->pushBufferSize);
  group->pushBufferSize += size;

  header->type = type;

  return header;
}

// `offset` is from the min corner
internal void
PushBitmap(render_group *group,
  loaded_bitmap *bitmap,
  v2 offset,
  v2 align,
  real32 entityZC = 1.0f,
  real32 alpha = 1.0f)
{
  render_entry_bitmap *entry = PushRenderElement(group, render_entry_bitmap);
  entry->entityBasis.offset = group->metersToPixels * offset + align;
  entry->entityBasis.basis = group->defaultBasis;
  entry->entityBasis.entityZC = entityZC;
  entry->bitmap = bitmap;
  entry->alpha = alpha;
}

internal void
Clear(render_group *group, v4 color)
{
  render_entry_clear *entry = PushRenderElement(group, render_entry_clear);
  entry->color = color;
}

internal render_entry_coordinate_system *
CoordinateSystem(render_group *group, v2 origin, v2 xAxis, v2 yAxis, v4 color)
{
  render_entry_coordinate_system *entry
    = PushRenderElement(group, render_entry_coordinate_system);

  entry->origin = origin;
  entry->xAxis = xAxis;
  entry->yAxis = yAxis;
  entry->color = color;

  return entry;
}

// `offset` is from the center
internal void
PushRect(render_group *group,
  v2 offset,
  v2 align,
  v2 dim,
  v4 color,
  real32 entityZC = 0.0f)
{
  render_entry_rectangle *entry
    = PushRenderElement(group, render_entry_rectangle);
  entry->entityBasis.offset = group->metersToPixels * offset + align
    - 0.5f * dim * group->metersToPixels;
  entry->entityBasis.basis = group->defaultBasis;
  entry->entityBasis.entityZC = entityZC;
  entry->color = color;
  entry->dim = dim * group->metersToPixels;
}

// `offset` is from the center
internal void
PushRectOutline(render_group *group, v2 offset, v2 align, v2 dim, v4 color)
{
  real32 thickness = 0.1f;
  v2 halfDim = 0.5f * dim;
  // Top and bottom
  PushRect(group,
    V2(offset.x, offset.y - halfDim.y),
    align,
    V2(dim.x, thickness),
    color);
  PushRect(group,
    V2(offset.x, offset.y + halfDim.y),
    align,
    V2(dim.x, thickness),
    color);

  // Left and right
  PushRect(group,
    V2(offset.x - halfDim.x, offset.y),
    align,
    V2(thickness, dim.y),
    color);
  PushRect(group,
    V2(offset.x + halfDim.x, offset.y),
    align,
    V2(thickness, dim.y),
    color);
}

internal v2
GetEntityMinCorner(render_entity_basis *entityBasis,
  v2 screenCenter,
  real32 metersToPixels)
{
  v3 entityP = entityBasis->basis->p;
  real32 entityZ = entityP.z;
  real32 zFudge = 1.0f + 0.1f * entityZ;

  v2 min
    = screenCenter + zFudge * entityP.xy * metersToPixels + entityBasis->offset;

  min.y += entityBasis->entityZC * entityZ * metersToPixels;

  return min;
}

internal void
RenderGroupToOutput(render_group *renderGroup, loaded_bitmap *outputTarget)
{
  real32 metersToPixels = renderGroup->metersToPixels;
  v2 screenCenter
    = 0.5f * v2{ (real32)outputTarget->width, (real32)outputTarget->height };

  for(uint32 baseAddr = 0; baseAddr < renderGroup->pushBufferSize;) {
    render_entry_header *header
      = (render_entry_header *)(renderGroup->pushBufferBase + baseAddr);

    switch(header->type) {
      case RenderEntryType_render_entry_clear: {
        render_entry_clear *entry = (render_entry_clear *)header;
        baseAddr += sizeof(render_entry_clear);

        DrawRectangle(outputTarget,
          V2(0, 0),
          V2((real32)outputTarget->width, (real32)outputTarget->height),
          entry->color);
      } break;

      case RenderEntryType_render_entry_rectangle: {
        render_entry_rectangle *entry = (render_entry_rectangle *)header;
        baseAddr += sizeof(render_entry_rectangle);
        v3 entityP = entry->entityBasis.basis->p;

        v2 min = GetEntityMinCorner(&entry->entityBasis,
          screenCenter,
          renderGroup->metersToPixels);

        DrawRectangle(outputTarget, min, min + entry->dim, entry->color);
      } break;

      case RenderEntryType_render_entry_bitmap: {
        render_entry_bitmap *entry = (render_entry_bitmap *)header;
        baseAddr += sizeof(render_entry_bitmap);

        v2 min = GetEntityMinCorner(&entry->entityBasis,
          screenCenter,
          renderGroup->metersToPixels);

        DrawBitmap(outputTarget, entry->bitmap, min, entry->alpha);
      } break;

      case RenderEntryType_render_entry_coordinate_system: {
        render_entry_coordinate_system *entry
          = (render_entry_coordinate_system *)header;
        baseAddr += sizeof(render_entry_coordinate_system);

        v2 min = entry->origin;
        v2 max = entry->origin + entry->xAxis + entry->yAxis;
        DrawRectangleSlowly(outputTarget,
          entry->origin,
          entry->xAxis,
          entry->yAxis,
          entry->color,
          entry->texture);

        v2 dim = V2(2, 2);
        v2 p = entry->origin;
        DrawRectangle(outputTarget, p, p + dim, entry->color);

        p = entry->origin + entry->xAxis;
        DrawRectangle(outputTarget, p, p + dim, entry->color);

        p = entry->origin + entry->yAxis;
        DrawRectangle(outputTarget, p, p + dim, entry->color);

        DrawRectangle(outputTarget, max, max + dim, entry->color);
#if 0
        for(uint32 i = 0; i < ArrayCount(entry->points); i++) {
          v2 p = entry->points[i];
          v2 min = entry->origin + p.x * entry->xAxis + p.y * entry->yAxis;
          DrawRectangle(outputTarget, min, min + dim, entry->color);
        }
#endif
      } break;

        InvalidDefaultCase;
    }
  }
}
