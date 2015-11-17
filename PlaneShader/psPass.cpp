// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psPass.h"
#include "psSolid.h"
#include "psCullGroup.h"
#include "psStateblock.h"
#include "psShader.h"
#include "bss-util/profiler.h"

using namespace planeshader;
using namespace bss_util;

psPass* psPass::CurPass = 0;

psPass::psPass() : _cam(&psCamera::default_camera), _renderables(0), _defaultrt(0), _renderlist(&_renderalloc), _solids(0), _cullgroups(0), _clear(false), _clearcolor(0)
{

}
psPass::~psPass()
{
  while(_renderables) _renderables->SetPass(0);
  while(_solids) _solids->SetPass(0);
  while(_cullgroups) _cullgroups->SetPass(0);
}

BSS_FORCEINLINE void r_adjust(sseVec& window, sseVec& winhold, sseVec& center, float& last, float adjust)
{
  if(last != adjust)
  {
    last=adjust;
    window=winhold;
    window*=adjust;
    window+=center;
  }
}

float BSS_FASTCALL r_gettotalz(psInheritable* p)
{
  if(p->GetParent())
    return p->GetPosition().z + r_gettotalz(p->GetParent());
  return p->GetPosition().z;
}

void psPass::Begin()
{
  PROFILE_FUNC();
  CurPass = this;
  auto& vp = _cam->GetViewPort();
  psRectiu realvp = { (unsigned int)bss_util::fFastRound(vp.left*_driver->rawscreendim.x), (unsigned int)bss_util::fFastRound(vp.top*_driver->rawscreendim.y), (unsigned int)bss_util::fFastRound(vp.right*_driver->rawscreendim.x), (unsigned int)bss_util::fFastRound(vp.bottom*_driver->rawscreendim.y) };
  _driver->PushCamera(_cam->GetPosition(), _cam->GetPivot()*psVec(_driver->rawscreendim), _cam->GetRotation(), realvp);

  if(_clear)
    _driver->Clear(_clearcolor);

  // We go through all the renderables and solids when the pass begins because we've already applied
  // the camera, and this allows you to do proper post-processing with immediate render commands.
  for(psRenderable* cur = _renderables; cur != 0; cur=cur->_llist.next)
    _sort(cur);

  // Go through the solids seperately, because they should be culled
  const psVec3D& p = _cam->GetPosition();
  BSS_ALIGN(16) psRect window = psRectRotate(realvp.left + p.x, realvp.top + p.y, realvp.right + p.x, realvp.bottom + p.y, _cam->GetRotation(), _cam->GetPivot()).BuildAABB();
  BSS_ALIGN(16) psRect winfixed = realvp;

  sseVec SSEwindow(window._ltrbarray);
  sseVec SSEwindow_center((SSEwindow+sseVec::Shuffle<0x4E>(SSEwindow))*sseVec(0.5f));
  sseVec SSEwindow_hold(SSEwindow - SSEwindow_center);

  sseVec SSEfixed(winfixed._ltrbarray);
  sseVec SSEfixed_center((SSEfixed+sseVec::Shuffle<0x4E>(SSEfixed))*sseVec(0.5f));
  sseVec SSEfixed_hold(SSEfixed - SSEfixed_center);

  float last;
  float lastfixed;
  FLAG_TYPE flags;
  float adjust;

  for(psSolid* solid = static_cast<psSolid*>(_solids); solid!=0; solid=static_cast<psSolid*>(solid->_llist.next))
  {
    flags=solid->GetAllFlags();
    if((flags&PSFLAG_DONOTCULL)!=0) { _sort(solid); continue; } // Don't cull if it has a DONOTCULL flag

    const psRect& rect = solid->GetBoundingRect(); // Recalculates the bounding rect
    adjust=r_gettotalz(solid);

    if(flags&PSFLAG_FIXED) // This is the fixed case
    {
      adjust += 1.0f;
      r_adjust(SSEfixed, SSEfixed_hold, SSEfixed_center, lastfixed, adjust);
      BSS_ALIGN(16) psRect rfixed(SSEfixed);
      if(rect.IntersectRect(rfixed)) _sort(solid);
    } else { // This is the default case
      adjust -= p.z;
      r_adjust(SSEwindow, SSEwindow_hold, SSEwindow_center, last, adjust);
      BSS_ALIGN(16) psRect rfixed(SSEwindow);
      if(rect.IntersectRect(rfixed)) _sort(solid);
    }
  }

  // Then we go through any specialized culling groups
  for(psCullGroup* group = _cullgroups; group != 0; group=group->next)
    group->Traverse(window._ltrbarray, p.z);

  // Go through our sorted list of renderables and queue them all.
  auto node = _renderlist.Front(); 
  auto next = node;
  while(node) {
    if(!(node->value->_internalflags&psRenderable::INTERNALFLAG_ACTIVE)) {
      next = node->next;
      node->value->_psort = 0;
      _renderlist.Remove(node);
      node = next;
    } else {
      _queue(node->value);
      node->value->_internalflags &= ~psRenderable::INTERNALFLAG_ACTIVE;
      node = node->next;
    }
  }
}

void psPass::End()
{
  FlushQueue();
  CurPass = 0;
  _driver->PopCamera();
  PROFILE_FUNC();
}

void psPass::Insert(psRenderable* r)
{
  PROFILE_FUNC();
  r->_pass = this;
  AltLLAdd<psRenderable, &psPass::GetRenderableAlt>(r, (r->_internalflags&psRenderable::INTERNALFLAG_SOLID)?_solids:_renderables);
}

void psPass::Remove(psRenderable* r)
{
  PROFILE_FUNC();
  AltLLRemove<psRenderable, &psPass::GetRenderableAlt>(r, (r->_internalflags&psRenderable::INTERNALFLAG_SOLID)?_solids:_renderables);
  if(r->_psort != 0) _renderlist.Remove(r->_psort);
  r->_psort = 0;
  r->_pass = 0;
}

void psPass::FlushQueue()
{
  if(_renderqueue.Length() > 0) // Don't render an empty render queue, which can happen if you call this manually or at the end of a pass that has nothing in it.
  {
    _renderqueue[0]->GetShader()->Activate();
    _driver->SetStateblock(!_renderqueue[0]->GetStateblock()?0:_renderqueue[0]->GetStateblock()->GetSB());
    _driver->SetRenderTargets(_renderqueue[0]->GetRenderTargets(), _renderqueue[0]->NumRT(), 0);

    if(_renderqueue.Length() == 1)
      _renderqueue[0]->_render();
    else
    {
      _renderqueue[0]->_renderbatchlist(_renderqueue, _renderqueue.Length());
    }
    _renderqueue.SetLength(0);
  }
}

void psPass::_queue(psRenderable* r)
{
  if(_renderqueue.Length() > 0 &&
    (_renderqueue[0]->_internaltype() != r->_internaltype() ||
    _renderqueue[0]->GetShader() != r->GetShader() ||
    _renderqueue[0]->GetStateblock() != r->GetStateblock() ||
    !_renderqueue[0]->_batch(r)))
    FlushQueue();
  _renderqueue.Add(r);
}

void psPass::_cullqueue(psRenderable* r)
{
  if(r->_internalflags&psRenderable::INTERNALFLAG_SOLID)
  {
    // TODO: implement culling
  }
  _queue(r);
}

void psPass::_sort(psRenderable* r)
{
  if(!r->_psort) r->_psort = _renderlist.Insert(r);
  r->_internalflags |= psRenderable::INTERNALFLAG_ACTIVE;
}

void psPass::_addcullgroup(psCullGroup* g)
{
  LLAdd<psCullGroup>(g, _cullgroups);
}
void psPass::_removecullgroup(psCullGroup* g)
{
  LLRemove<psCullGroup>(g, _cullgroups);
}