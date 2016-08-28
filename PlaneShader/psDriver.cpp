// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psDriver.h"
#include "psShader.h"
#include "psStateblock.h"
#include "psRenderable.h"
#include "psTex.h"

using namespace planeshader;

psVec3D BSS_FASTCALL psDriver::FromScreenSpace(const psVec& point, float z) const
{ 
  psVec pt = (point - (GetBackBuffer()->GetRawDim() / 2u)) * (-ReversePoint(VEC3D_ZERO).z + z);
  return ReversePoint(psVec3D(pt.x, pt.y, 1.0f + z));
}

#include "psEngine.h"

psBatchObj* BSS_FASTCALL psDriver::DrawBatchBegin(psShader* shader, void* stateblock, psFlag flags, psBufferObj* verts, psBufferObj* indices, PRIMITIVETYPE rendermode, const float(&transform)[4][4], uint32_t reserve)
{
  uint32_t snapshot = GetSnapshot(); // Snapshot our driver state
  if(_jobstack.Length() > 0)
  {
    auto& last = _jobstack.Back(); // Can this be batch rendered?
    if(!(flags&PSFLAG_DONOTBATCH) && 
      last.buffer.verts == verts &&
      last.buffer.indices == indices &&
      last.buffer.mode == rendermode &&
      last.snapshot == snapshot &&
      (last.flags&PSFLAG_FIXED) == (flags&PSFLAG_FIXED) &&
      last.shader == shader &&
      last.stateblock == stateblock &&
      &last.transform == &transform)
    {
      if((last.buffer.vert + last.buffer.nvert + reserve)*verts->element <= verts->capacity)
        return &last; // If we can handle the necessary buffer reserve, batch render with this
      Flush(); // Otherwise, we have to flush everything and then create an entirely new buffer
      snapshot = GetSnapshot(); // Flush invalidates all snapshots
    }
    else
      last.buffer.verts->length += last.buffer.nvert; // we don't manage the index buffer length because it's often managed seperately
  }

  if((verts->length + reserve)*verts->element > verts->capacity) // even if we didn't try to batch render we still have to check if the buffer itself might be overrun.
  {
    Flush();
    snapshot = GetSnapshot(); // Flush invalidates all snapshots
  }
  assert(reserve*verts->element <= verts->capacity);

  _jobstack.AddConstruct(transform);
  psBatchObj& o = _jobstack.Back();
  if(!verts->mem) verts->mem = LockBuffer(verts->buffer, LOCK_WRITE_DISCARD); // Note: we DO NOT LOCK the indices here because most of the time you don't want to.
  o.buffer.verts = verts;
  o.buffer.vert = verts->length;
  o.buffer.nvert = 0;
  o.buffer.indices = indices;
  o.buffer.indice = !indices ? 0 : indices->length;
  o.buffer.nindice = 0;
  o.buffer.mode = rendermode;
  o.shader = shader;
  o.stateblock = stateblock;
  o.flags = flags;
  o.snapshot = snapshot;
  return &o;
}

// Flushes the batch job queue
/*void BSS_FASTCALL psDriver::Flush(bool preserve)
{
  obj.shader->Activate();
  SetStateblock(obj.stateblock);
  UnlockBuffer(obj.buffer->verts->buffer);
  if(obj.buffer->nvert > 0)
    Draw(obj.buffer, obj.flags, obj.transform);
  obj.offset = 0; // Zeroing these allows this function to be called from within another draw function to reset the batch call (it still has to lock the buffer again).
  obj.indices = 0;
  obj.buffer->nvert = 0;
}*/

psBufferObj* BSS_FASTCALL psDriver::CreateBufferObj(psBufferObj* target, uint32_t capacity, uint32_t element, uint32_t usage, const void* initdata)
{
  assert(target != 0);
  target->buffer = CreateBuffer(capacity, element, usage, initdata);
  target->mem = 0;
  target->element = element;
  target->length = 0;
  target->capacity = capacity*element;
  return target; // We return the same pointer we passed in due to restrictions on function calls in static variables (see psVector.cpp)
}

psBatchObj* BSS_FASTCALL psDriver::DrawArray(psShader* shader, const psStateblock* stateblock, void* data, uint32_t num, psBufferObj* vbuf, psBufferObj* ibuf, PRIMITIVETYPE mode, psFlag flags, const float(&transform)[4][4])
{
  psBatchObj* obj = DrawBatchBegin(shader, !stateblock ? 0 : stateblock->GetSB(), flags, vbuf, ibuf, mode, transform);
  psBufferObj* verts = obj->buffer.verts;

  for(uint32_t i = (num + obj->buffer.vert + obj->buffer.nvert)*verts->element; i > verts->capacity; i -= verts->capacity)
  {
    int n = (verts->capacity / verts->element) - obj->buffer.vert - obj->buffer.nvert;
    memcpy(obj->buffer.get(), data, n*verts->element);
    num -= n;
    obj->buffer.nvert += n;
    data = ((char*)data) + (n*verts->element);
    obj = FlushPreserve();
    verts = obj->buffer.verts;
  }
  memcpy(obj->buffer.get(), data, num*verts->element); // We can do this because we know num+_lockedptcount does not exceed MAX_POINTS
  obj->buffer.nvert += num;
  return obj;
}

void BSS_FASTCALL psDriver::MergeClipRect(const psRect& rect)
{
  PushClipRect(PeekClipRect().GenerateIntersection(rect));
}