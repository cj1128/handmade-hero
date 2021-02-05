#ifndef HANDMADE_RENDER_GROUP_H
#define HANDMADE_RENDER_GROUP_H

struct render_basis {
  v3 p;
};

enum render_entry_type {
  RenderEntryType_render_entry_clear,
  RenderEntryType_render_entry_rectangle,
  RenderEntryType_render_entry_bitmap,
};

struct render_entry_header {
  render_entry_type type;
};

struct render_entry_clear {
  render_entry_header header;
  v4 color;
};

// position of the min corner
struct render_entity_basis {
  render_basis *basis;
  v2 offset;
  real32 entityZC;
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

  real32 alpha;
  loaded_bitmap *bitmap;
};

struct render_group {
  real32 metersToPixels;
  render_basis *defaultBasis;
  uint32 pieceCount;

  uint32 maxPushBufferSize;
  uint32 pushBufferSize;
  uint8 *pushBufferBase;
};

#endif
