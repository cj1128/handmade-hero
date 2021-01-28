internal render_group *
AllocateRenderGroup(memory_arena *arena,
  uint32 maxPushBufferSize,
  real32 metersToPixels)
{
  render_group *result = PushStruct(arena, render_group);
  render_basis *defaultBasis = PushStruct(arena, render_basis);

  result->defaultBasis = defaultBasis;
  result->pieceCount = 0;
  result->metersToPixels = metersToPixels;

  result->maxPushBufferSize = maxPushBufferSize;
  result->pushBufferSize = 0;
  result->pushBufferBase = (uint8 *)PushSize(arena, maxPushBufferSize);

  return result;
}

internal uint8 *
PushRenderElement(render_group *group, uint32 size)
{
  Assert(group->pushBufferSize + size <= group->maxPushBufferSize);

  uint8 *result = group->pushBufferBase + group->pushBufferSize;
  group->pushBufferSize += size;

  return result;
}

internal void
PushPiece(render_group *group,
  loaded_bitmap *bitmap,
  v2 offset,
  v2 align,
  v2 halfDim,
  v3 color,
  real32 entityZC,
  real32 alpha)
{
  render_piece *piece
    = (render_piece *)PushRenderElement(group, sizeof(render_piece));
  piece->bitmap = bitmap;
  piece->offset = group->metersToPixels * offset + align;
  piece->basis = group->defaultBasis;
  piece->alpha = alpha;
  piece->color = color;
  piece->halfDim = halfDim;
  piece->entityZC = entityZC;
}

// `offset` is from the bottom-left
internal void
PushBitmap(render_group *group,
  loaded_bitmap *bitmap,
  v2 offset,
  v2 align,
  real32 entityZC = 1.0f,
  real32 alpha = 1.0f)
{
  PushPiece(group,
    bitmap,
    offset,
    align,
    V2(0, 0),
    V3(0, 0, 0),
    entityZC,
    alpha);
}

// `offset` is from the center
internal void
PushRect(render_group *group,
  v2 offset,
  v2 align,
  v2 halfDim,
  v3 color,
  real32 alpha = 1.0f,
  real32 entityZC = 1.0f)
{
  PushPiece(group, NULL, offset, align, halfDim, color, alpha, entityZC);
}

// `offset` is from the center
internal void
PushRectOutline(render_group *group, v2 offset, v2 align, v2 halfDim, v3 color)
{
  real32 thickness = 0.1f;
  // Top and bottom
  PushRect(group,
    v2{ offset.x, offset.y - halfDim.y },
    align,
    v2{ halfDim.x, thickness },
    v3{ 0.0f, 0, 1.0f });
  PushRect(group,
    v2{ offset.x, offset.y + halfDim.y },
    align,
    v2{ halfDim.x, thickness },
    v3{ 0.0f, 0, 1.0f });

  // Left and right
  PushRect(group,
    v2{ offset.x - halfDim.x, offset.y },
    align,
    v2{ thickness, halfDim.y },
    v3{ 0.0f, 0, 1.0f });
  PushRect(group,
    v2{ offset.x + halfDim.x, offset.y },
    align,
    v2{ thickness, halfDim.y },
    v3{ 0.0f, 0, 1.0f });
}
