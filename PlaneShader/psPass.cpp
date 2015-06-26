// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "bss-util/profiler.h"
#include "psPass.h"
#include "psSolid.h"
#include "psCullGroup.h"

using namespace planeshader;
using namespace bss_util;

psPass* psPass::_curpass = 0;

psPass::psPass() : _cam(&psCamera::default_camera), _renderables(0), _defaultrt(0), _renderlist(&_renderalloc)
{

}
psPass::~psPass()
{
  while(_renderables) _renderables->SetPass((unsigned short)-1);
  while(_solids) _solids->SetPass((unsigned short)-1);
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
  _curpass = this;
  auto& vp = _cam->GetViewPort();
  psRectiu realvp = { (unsigned int)bss_util::fFastRound(vp.left*_driver->screendim.x), (unsigned int)bss_util::fFastRound(vp.top*_driver->screendim.y), (unsigned int)bss_util::fFastRound(vp.right*_driver->screendim.x), (unsigned int)bss_util::fFastRound(vp.bottom*_driver->screendim.y) };
  _driver->ApplyCamera(_cam->GetPosition(), _cam->GetPivot(), _cam->GetRotation(), realvp);
  _driver->SetDefaultRenderTarget(_defaultrt);

  // We go through all the renderables and solids when the pass begins because we've already applied
  // the camera, and this allows you to do proper post-processing with immediate render commands.
  psRenderable* cur = _renderables;
  while(cur)
  {
    _sort(cur);
    cur=cur->_llist.next;
  }

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

  for(psSolid* solid = static_cast<psSolid*>(_solids); cur!=0; cur=static_cast<psSolid*>(cur->_llist.next))
  {
    flags=solid->GetTotalFlags();
    if((flags&PSFLAG_DONOTCULL)!=0) { _sort(solid); continue; } // Don't cull if it has a DONOTCULL flag

    const psRect& rect = solid->GetBoundingRect(); // Recalculates the bounding rect
    adjust=1.0f+r_gettotalz(solid);

    if(flags&PSFLAG_FIXED) // This is the fixed case
    {
      r_adjust(SSEfixed, SSEfixed_hold, SSEfixed_center, lastfixed, adjust);
      if(rect.IntersectRect(winfixed)) _sort(solid);
    } else { // This is the default case
      adjust+=p.z;
      r_adjust(SSEwindow, SSEwindow_hold, SSEwindow_center, last, adjust);
      if(rect.IntersectRect(winfixed)) _sort(solid);
    }
  }

  // Go through our sorted list of renderables and queue them all.
  auto node = _renderlist.Front(); 
  auto next = node;
  while(node) {
    if(!(node->value->_internalflags&1)) {
      next = node->next;
      _renderlist.Remove(node);
      node = next;
    } else {
      _queue(node->value);
      node->value->_internalflags = (~1)&node->value->_internalflags;
      node = node->next;
    }
  }
}

void psPass::End()
{
  _curpass = 0;
  FlushQueue();
  PROFILE_FUNC();
}

void psPass::Insert(psRenderable* r)
{
  PROFILE_FUNC();
  AltLLAdd<psRenderable, &psPass::GetRenderableAlt>(r, (r->_internalflags&psRenderable::INTERNALFLAG_SOLID)?_solids:_renderables);
}

void psPass::Remove(psRenderable* r)
{
  PROFILE_FUNC();
  AltLLRemove<psRenderable, &psPass::GetRenderableAlt>(r, (r->_internalflags&psRenderable::INTERNALFLAG_SOLID)?_solids:_renderables);
  if(r->_psort != 0) _renderlist.Remove(r->_psort);
  r->_psort = 0;
}

void psPass::FlushQueue()
{
  if(_renderqueue.Length() == 1)
    _renderqueue[0]->_render();
  else if(_renderqueue.Length() > 1) // Don't render an empty render queue, which can happen if you call this manually or at the end of a pass that has nothing in it.
    _renderqueue[0]->_renderbatch(_renderqueue);
}

void psPass::_queue(psRenderable* r)
{
  if(_renderqueue.Length() > 0 && !_renderqueue[0]->_batch(r))
    FlushQueue();
  _renderqueue.Add(r);
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