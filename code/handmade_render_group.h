#ifndef HANDMADE_RENDER_GROUP_H
#define HANDMADE_RENDER_GROUP_H

struct render_basis {
  v3 p;
};

struct render_piece {
  render_basis *basis;
  loaded_bitmap *bitmap;
  v2 halfDim;
  v3 color;
  real32 alpha;
  v2 offset;
  real32 entityZC;
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
