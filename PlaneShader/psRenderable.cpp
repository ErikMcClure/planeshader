// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#include "psRenderable.h"
#include "psEngine.h"
#include "psLayer.h"
#include "psTex.h"
#include "bss-util/profiler.h"

using namespace planeshader;

psRenderable::psRenderable(const psRenderable& copy) : _flags(copy._flags), _layer(copy._layer), _internalflags(copy._internalflags), _zorder(copy._zorder),
  _stateblock(copy._stateblock), _shader(psShader::CreateShader(copy._shader)), _psort(0)
{
  _llist.next = _llist.prev = 0;
  _copyinsert(copy);
}
psRenderable::psRenderable(psRenderable&& mov) : _flags(mov._flags), _layer(mov._layer), _internalflags(mov._internalflags), _zorder(mov._zorder),
  _stateblock(std::move(mov._stateblock)), _shader(std::move(mov._shader)), _psort(0)
{ 
  _llist.next=_llist.prev=0;
  _copyinsert(mov);

  if(mov._layer != 0) // We deliberately don't call SetPass(0), because we don't WANT to propagate this change down to mov's children - there's no point, because the children aren't being moved!
    mov._layer->Remove(&mov);
  mov._layer = 0;
}

psRenderable::psRenderable(psFlag flags, int zorder, psStateblock* stateblock, psShader* shader, psLayer* pass) : _flags(flags), _zorder(zorder),
  _stateblock(stateblock), _shader(psShader::CreateShader(shader)), _layer(0), _internalflags(0), _psort(0)
{ 
  _llist.next=_llist.prev=0;
  SetPass(pass);
}

psRenderable::~psRenderable() { _destroy(); }
void psRenderable::Render(const psTransform2D& parent)
{
  if(psLayer::CurLayers.Length() > 0)
  {
    if(_layer != 0 && _layer != psLayer::CurLayers.Peek())
      _layer->Defer(this, parent);
    else if(!Cull(parent))
      _render(parent);
  }
  else
    assert(false);
}
void psRenderable::SetZOrder(int zorder)
{ 
  _zorder = zorder;
  _invalidate();
}
void psRenderable::SetPass()
{
  SetPass(psEngine::Instance()->GetLayer(0));
}
void psRenderable::SetPass(psLayer* layer)
{
  PROFILE_FUNC();
  if(layer == _layer || !psEngine::Instance()) return; // If a renderable is deleted after the engine is deleted, make sure we don't blow everything up
  if(_layer != 0)
    _layer->Remove(this);
  _layer = layer;
}

void psRenderable::SetStateblock(psStateblock* stateblock)
{
  PROFILE_FUNC();
  _stateblock = stateblock;
}

void psRenderable::_destroy()
{
  SetPass(0);
}

void psRenderable::_invalidate()
{
  if(_psort!=0 && ((_psort->prev!=0 && StandardCompare(_psort->prev->value.first, this)>0) || (_psort->next && StandardCompare(this, _psort->next->value.first)<0)))
  { // We only invalidate if the new parameters actually invalidate the object's position.
    _layer->_renderlist.Remove(_psort);
    _psort = 0;
  }
}

psRenderable& psRenderable::operator =(const psRenderable& right)
{
  PROFILE_FUNC();
  _destroy();
  _flags=right._flags;
  _internalflags=right._internalflags;
  _zorder=right._zorder;
  _stateblock=right._stateblock;
  _shader=psShader::CreateShader(right._shader);
  _psort = 0;
  _llist.next=_llist.prev=0;
  _layer = right._layer; // We can do this because _destroy already removed us from any previous pass
  _copyinsert(right);
  return *this;
}

psRenderable& psRenderable::operator =(psRenderable&& right)
{
  PROFILE_FUNC();
  _destroy();
  _flags=right._flags;
  _internalflags=right._internalflags;
  _zorder=right._zorder;
  _stateblock=std::move(right._stateblock);
  _shader=std::move(right._shader);
  _psort = 0;

  _llist.next=_llist.prev=0;
  _layer = right._layer;
  _copyinsert(right);

  if(right._layer != 0) // We should have already removed right's children, but just in case we avoid calling SetPass anyway.
    right._layer->Remove(&right);
  right._layer = 0;
  return *this;
}

void psRenderable::_copyinsert(const psRenderable& r)
{
  if(r._layer != 0 && (r._llist.prev != 0 || r._layer->_renderables == &r)) // Check if r was in the internal pass list. If so, add ourselves to it.
    r._layer->Insert(this);
}
