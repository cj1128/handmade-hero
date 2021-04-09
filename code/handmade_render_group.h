#ifndef HANDMADE_RENDER_GROUP_H
#define HANDMADE_RENDER_GROUP_H

struct environment_map {
  loaded_bitmap *lod[4];
};

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
  v2 offset;
  f32 entityZC;
};

struct render_entry_rectangle {
  render_entity_basis entityBasis;

  v2 dim;
  v4 color;
};

struct render_entry_bitmap {
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
