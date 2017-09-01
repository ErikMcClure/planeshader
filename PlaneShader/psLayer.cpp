// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#include "psLayer.h"
#include "psSolid.h"
#include "psCullGroup.h"
#include "psStateblock.h"
#include "psShader.h"
#include "psEngine.h"
#include "bss-util/profiler.h"

using namespace planeshader;
using namespace bss;

bss::Stack<psLayer*> psLayer::CurLayers;

psLayer::~psLayer()
{
  while(_renderables) _renderables->SetPass(0);
}

void psLayer::Push()
{
  PROFILE_FUNC();
  CurLayers.Push(this);
  _applytop();
  if(_clear)
  {
    auto targets = _driver->GetRenderTargets();
    for(uint8_t i = 0; i < targets.second; ++i)
      _driver->Clear(targets.first[i], _clearcolor);
  }
  // We go through all the renderables and solids when the pass begins because we've already applied
  // the camera, and this allows you to do proper post-processing with immediate render commands.
  for(psRenderable* cur = _renderables; cur != 0; cur = cur->_llist.next)
    cur->Render(0);
}
void psLayer::_applytop()
{
  if(!CurLayers.Length())
    return;
  psLayer& cur = *CurLayers.Peek();
  if(cur._dpi.x != 0 && cur._dpi.y != 0)
    _driver->SetDPIScale(psVec(cur._dpi) / psVec(BASE_DPI));
  if(cur._targets.Capacity() > 0)
    _driver->SetRenderTargets(cur.GetTargets(), cur.NumTargets());
  if(cur._cam != 0)
  {
    auto targets = _driver->GetRenderTargets();
    if(targets.first)
      cur._cam->Apply(targets.first[0]->GetRawDim());
  }
}

void psLayer::Pop()
{
  // Go through our sorted list of renderables and render them all.
  auto node = _renderlist.Front();
  auto next = node;
  while(node)
  {
    if(!(node->value->_internalflags&psRenderable::INTERNALFLAG_ACTIVE))
    {
      next = node->next;
      node->value->_psort = 0;
      _renderlist.Remove(node);
      node = next;
    }
    else
    {
      node->value->_render(psTransform2D::Zero);
      node->value->_internalflags &= ~psRenderable::INTERNALFLAG_ACTIVE;
      node = node->next;
    }
  }

  _driver->Flush();
  CurLayers.Pop();
  _applytop();
  PROFILE_FUNC();
}

void psLayer::Insert(psRenderable* r)
{
  PROFILE_FUNC();
  if(r->_layer != 0 && r->_layer != this) r->_layer->Remove(r);
  r->_layer = this;
  AltLLAdd<psRenderable, &psRenderable::GetRenderableAlt>(r, _renderables);
}

void psLayer::Remove(psRenderable* r)
{
  PROFILE_FUNC();
  if(r->_layer == this)
  {
    AltLLRemove<psRenderable, &psRenderable::GetRenderableAlt>(r, _renderables); // This will only remove the renderable if it was actually in our list, otherwise it does nothing.
    r->_llist.prev = r->_llist.next = 0;
    if(r->_psort != 0) _renderlist.Remove(r->_psort);
    r->_psort = 0;
  }
}
void psLayer::Defer(psRenderable* r, const psTransform2D& parent)
{
  _defer.AddConstruct(r, parent);
}

void psLayer::SetTargets(psTex* const* targets, uint8_t num)
{
  _targets.SetCapacity(num);
  for(uint8_t i = 0; i < num; ++i)
    _targets[i] = targets[i];
}

void psLayer::_sort(psRenderable* r)
{
  if(r->_layer != this)
  {
    if(r->_layer != 0)
      r->_layer->Remove(r);
    r->_layer = this;
  }
  if(!r->_psort) r->_psort = _renderlist.Insert(r);
  r->_internalflags |= psRenderable::INTERNALFLAG_ACTIVE;
}
void psLayer::_render(const psTransform2D& parent)
{
  Push();
  Pop();
}

bool psLayer::Cull(psSolid* solid, const psTransform2D* parent) const
{
  assert(psLayer::CurLayer() == this);
  if((solid->GetFlags()&PSFLAG_DONOTCULL) != 0)
    return false; // Don't cull if it has a DONOTCULL flag
  return _cull.Cull(solid->GetBoundingRect((!parent) ? (psTransform2D::Zero) : (*parent)), solid->GetPosition().z + (!parent ? 0 : parent->position.z), _cull.z, solid->GetFlags());
}
