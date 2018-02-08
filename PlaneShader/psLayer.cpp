// Copyright ©2018 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#include "psEngine.h"
#include "psLayer.h"
#include "psSolid.h"
#include "psCullGroup.h"
#include "psStateblock.h"
#include "psShader.h"
#include "bss-util/profiler.h"

using namespace planeshader;
using namespace bss;

bss::Stack<psLayer*> psLayer::CurLayers;

psLayer::psLayer() : _cam(0), _renderlist(&psEngine::Instance()->NodeAlloc), _renderables(0), _dpi(0, 0), _clear(false) {}
psLayer::psLayer(psTex* const* targets, uint8_t num) : _cam(0), _renderlist(&psEngine::Instance()->NodeAlloc), _renderables(0), _dpi(0, 0), _clear(false) { SetTargets(targets, num); }
psLayer::psLayer(psLayer&& mov) : psRenderable(std::move(mov)), _cam(std::move(mov._cam)), _cull(mov._cull), _renderables(mov._renderables), _dpi(mov._dpi),
_clearcolor(mov._clearcolor), _clear(mov._clear), _defer(std::move(mov._defer)), _renderlist(std::move(mov._renderlist)), _targets(std::move(mov._targets))
{
  mov._renderables = 0;
  mov._dpi = { 0,0 };
}

psLayer& psLayer::operator=(psLayer&& mov)
{
  while(_renderables) _renderables->SetPass(0);

  _cam = std::move(mov._cam);
  _cam = mov._cam;
  _renderables = mov._renderables;
  _dpi = mov._dpi;
  _clearcolor = mov._clearcolor;
  _clear = mov._clear;
  _defer = std::move(mov._defer);
  _renderlist = std::move(mov._renderlist);
  _targets = std::move(mov._targets);
  
  mov._renderables = 0;
  mov._dpi = { 0,0 };
  return *this;
}
psLayer::~psLayer()
{
  while(_renderables) _renderables->SetPass(0);
  for(auto p = _renderlist.Front(); p != 0; p = p->next)
    p->value.first->_psort = 0; // Make sure any deferred renderables don't still point to this tree.
  _renderlist.Clear();
}

void psLayer::Push(const psTransform2D& parent)
{
  PROFILE_FUNC();
  if(!CurLayers.Length() || CurLayers.Peek() != this)
  {
    CurLayers.Push(this);
    _applytop();
  }
  if(_clear)
  {
    auto targets = _driver->GetRenderTargets();
    for(uint8_t i = 0; i < targets.second; ++i)
      _driver->Clear(targets.first[i], _clearcolor);
  }

  // Insert all deferred renderables into the tree. Don't clear the defered queue yet, because it's storing our transforms
  for(auto& r : _defer)
    _sort(r.first, r.second);

  // Insert all renderables into the tree
  for(psRenderable* cur = _renderables; cur != 0; cur = cur->_llist.next)
    _sort(cur, parent);
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
      cur._cam->Apply(targets.first[0]->GetRawDim(), cur._cull);
    else if(CurLayers.Length() > 1)
      cur._cull = CurLayers[CurLayers.Length() - 2]->_cull;
    else
      assert(false);
  }
  else if(CurLayers.Length() > 1)
    cur._cull = CurLayers[CurLayers.Length() - 2]->_cull;
  else // All root layers must have cameras
    assert(false);
}

void psLayer::Pop()
{
  assert(CurLayers.Peek() == this);
  // Go through our sorted list of renderables and render them all.
  auto node = _renderlist.Front();
  auto next = node;
  while(node)
  {
    if(!(node->value.first->_internalflags&psRenderable::INTERNALFLAG_ACTIVE))
    {
      next = node->next;
      node->value.first->_psort = 0;
      _renderlist.Remove(node);
      node = next;
    }
    else
    {
      if(!node->value.first->Cull(*node->value.second))
        node->value.first->_render(*node->value.second);
      node->value.first->_internalflags &= ~psRenderable::INTERNALFLAG_ACTIVE;
      node = node->next;
    }
  }

  _driver->Flush();
  _defer.Clear(); // Now we can clear the deferred array because we aren't using the transforms anymore
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

void psLayer::_sort(psRenderable* r, const psTransform2D& t)
{
  if(r->_layer != this)
  {
    if(r->_layer != 0)
      r->_layer->Remove(r);
    r->_layer = this;
  }
  if(!r->_psort) r->_psort = _renderlist.Insert(std::pair<psRenderable*, const psTransform2D*>(r, &t));
  r->_internalflags |= psRenderable::INTERNALFLAG_ACTIVE;
}
void psLayer::_render(const psTransform2D& parent)
{
  Push(parent);
  Pop();
}

psLayer* psLayer::CurLayer() 
{ 
  return !CurLayers.Length() ? nullptr : CurLayers.Peek();
}
psCamera* psLayer::CurCamera()
{
  size_t i = CurLayers.Length();
  while(i > 0)
    if(psCamera* cam = CurLayers[--i]->_cam)
      return cam;
  return 0;
}

bool psLayer::Cull(psSolid* solid, const psTransform2D* parent) const
{
  assert(psLayer::CurLayer() == this);
  if((solid->GetFlags()&PSFLAG_DONOTCULL) != 0)
    return false; // Don't cull if it has a DONOTCULL flag
  return _cull.Cull(solid->GetBoundingRect((!parent) ? (psTransform2D::Zero) : (*parent)), solid->GetPosition().z + (!parent ? 0 : parent->position.z), _cull.z, solid->GetFlags());
}
