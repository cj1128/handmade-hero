#ifndef HANDMADE_RENDER_GROUP_H
#define HANDMADE_RENDER_GROUP_H

struct render_basis {
  v3 p;
};

enum render_entry_type {
  RenderEntryType_render_entry_clear,
  RenderEntryType_render_entry_rectangle,
  RenderEntryType_render_entry_bitmap,
  RenderEntryType_render_entry_coordinate_system,
};

struct render_entry_header {
  render_entry_type type;
};

struct render_entry_coordinate_system {
  render_entry_header header;
  v2 origin;
  v2 xAxis;
  v2 yAxis;
  v4 color;
  loaded_bitmap *texture;
  v2 points[16];
};

struct render_entry_clear {
  render_entry_header header;
  v4 color;
};

// position of the min corner
struct render_entity_basis {
  render_basis *basis;
  v2 offset;
  f32 entityZC;
};

struct render_entry_rectangle {
  render_entry_header header;
  render_entity_basis entityBasis;

  v2 dim;
  v4 color;
};

struct render_entry_bitmap {
  render_entry_header header;
  render_entity_basis entityBasis;

  f32 alpha;
  loaded_bitmap *bitmap;
};

struct render_group {
  f32 metersToPixels;
  render_basis *defaultBasis;
  u32 pieceCount;

  u32 maxPushBufferSize;
  u32 pushBufferSize;
  u8 *pushBufferBase;
};

#endif
