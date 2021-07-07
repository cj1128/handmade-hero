#ifndef HANDMADE_RENDER_GROUP_H
#define HANDMADE_RENDER_GROUP_H

/*
  1) Y always goes upward, X to the right

  2) All bitmaps including the render target are assumed to be bottom-up,
  (meaning the first row pointer points to the bottom-most row when viewed on
  the screen)

  3) Unless otherwise specified, all inputs to the renderer are in world
  coordinate (*meters*), NOT pixels. Anything that is in pixel values will be
  explicitly marked as such.

  4) Z is a special coordinate because it is broken up into discrete slices. and
  the renderer actually understands these slices. Z Slices are what control the
  _scaling_ of things, where Z offsets inside a slice are what control Y
  offseting

  5) All colors values specified to the renderer as V4s are in NON-premultiplied
  alpha.
    1, 1, 1, 0.5 -> 0.5, 0.5, 0.5, 0.5
*/

struct loaded_bitmap {
  void *memory;
  i32 width, height;
  i32 pitch;

  v2 alignPercentage;

  f32 widthOverHeight;
};

struct environment_map {
  loaded_bitmap lod[4];
  f32 z;
};

struct render_basis {
  v3 p;
};

enum render_entry_type {
  RenderEntryType_render_entry_clear,
  RenderEntryType_render_entry_rectangle,
  RenderEntryType_render_entry_bitmap,
  RenderEntryType_render_entry_coordinate_system,
  RenderEntryType_render_entry_saturation,
};

struct render_entry_header {
  render_entry_type type;
};

struct render_entry_saturation {
  f32 level;
};

struct render_entry_coordinate_system {
  v2 origin;
  v2 xAxis;
  v2 yAxis;
  v4 color;
  loaded_bitmap *texture;
  loaded_bitmap *normalMap;
  environment_map *top;
  environment_map *middle;
  environment_map *bottom;
};

struct render_entry_clear {
  v4 color;
};

// position of the min corner
struct render_entity_basis {
  render_basis *basis;
  v3 offset;
};

struct render_entry_rectangle {
  render_entity_basis entityBasis;

  v2 dim;
  v4 color;
};

struct render_entry_bitmap {
  render_entity_basis entityBasis;
  v4 color;
  loaded_bitmap *bitmap;
  v2 size;
};

struct render_group {
  render_basis *defaultBasis;
  u32 pieceCount;

  u32 maxPushBufferSize;
  u32 pushBufferSize;
  u8 *pushBufferBase;

  f32 globalAlpha;
};

// Render APIs
#if 0
internal void PushBitmap(render_group *group,
  loaded_bitmap *bitmap,
  v2 offset,
  v2 align,
  f32 entityZC = 1.0f,
  f32 alpha = 1.0f);

internal void Clear(render_group *group, v4 color);

internal void PushRect(render_group *group,
  v2 offset,
  v2 align,
  v2 dim,
  v4 color,
  f32 entityZC = 0.0f);

internal void
PushRectOutline(render_group *group, v2 offset, v2 align, v2 dim, v4 color);
#endif

#endif
