// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psDriver.h"
#include "psShader.h"
#include "psStateblock.h"
#include "psRenderable.h"

using namespace planeshader;

void BSS_FASTCALL psDriver::DrawBatchBegin(psBatchObj& obj, psFlag flags, psVertObj* buffer, const float(&transform)[4][4])
{
  new(&obj) psBatchObj(flags, transform, buffer, 0, 0, LockBuffer(buffer->verts, LOCK_WRITE_DISCARD));
  obj.buffer->nvert = 0;
}

// Ends a batch segment, rendering if necessary.
void BSS_FASTCALL psDriver::DrawBatchEnd(psBatchObj& obj)
{
  UnlockBuffer(obj.buffer->verts);
  if(obj.buffer->nvert > 0)
    Draw(obj.buffer, obj.flags, obj.transform);
  obj.offset = 0; // Zeroing these allows this function to be called from within another draw function to reset the batch call (it still has to lock the buffer again).
  obj.indices = 0;
  obj.buffer->nvert = 0;
}