// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psPass.h"
#include "psSolid.h"
#include "psCullGroup.h"
#include "psStateblock.h"
#include "psShader.h"
#include "psEngine.h"
#include "bss-util/profiler.h"

using namespace planeshader;
using namespace bss_util;

psPass* psPass::CurPass = 0;

psPass::psPass(psMonitor* monitor) : _cam(&psCamera::default_camera), _renderables(0), _defaultrt(0), _renderlist(&_renderalloc), _clear(false),
  _clearcolor(0), _dpi(0), _monitor(monitor)
{

}
psPass::~psPass()
{
  while(_renderables) _renderables->SetPass(0);
}

void psPass::Begin()
{
  PROFILE_FUNC();
  CurPass = this;
  _driver->SetDPIScale(psVec(GetDPI())/psVec(psGUIManager::BASE_DPI));
  auto rt = GetRenderTarget();
  if(!rt) return;

  const psRect& window = _cam->Apply(*rt);

  if(_clear)
    _driver->Clear(_clearcolor);

  // We go through all the renderables and solids when the pass begins because we've already applied
  // the camera, and this allows you to do proper post-processing with immediate render commands.
  for(psRenderable* cur = _renderables; cur != 0; cur = cur->_llist.next)
    cur->Render(0);
}

void psPass::End()
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
      node->value->_render(psParent::Zero);
      node->value->_internalflags &= ~psRenderable::INTERNALFLAG_ACTIVE;
      node = node->next;
    }
  }

  _driver->Flush();
  CurPass = 0;
  _driver->PopCamera();
  PROFILE_FUNC();
}
psVeciu psPass::GetDPI()
{
  psVeciu dpi = _dpi;
  if(!_dpi.x)
    _dpi.x = !_monitor ? psEngine::Instance()->GetGUI().dpi.x : _monitor->dpi.x;
  if(!_dpi.y)
    _dpi.y = !_monitor ? psEngine::Instance()->GetGUI().dpi.y : _monitor->dpi.y;
  return dpi;
}
psTex* const* psPass::GetRenderTarget()
{
  return !_defaultrt ? _monitor->GetBackBuffer() : &_defaultrt;
}

void psPass::Insert(psRenderable* r)
{
  PROFILE_FUNC();
  if(r->_pass != 0 && r->_pass != this) r->_pass->Remove(r);
  r->_pass = this;
  AltLLAdd<psRenderable, &psPass::GetRenderableAlt>(r, _renderables);
}

void psPass::Remove(psRenderable* r)
{
  PROFILE_FUNC();
  if(r->_pass == this)
  {
    AltLLRemove<psRenderable, &psPass::GetRenderableAlt>(r, _renderables); // This will only remove the renderable if it was actually in our list, otherwise it does nothing.
    r->_llist.prev = r->_llist.next = 0;
    if(r->_psort != 0) _renderlist.Remove(r->_psort);
    r->_psort = 0;
  }
}
void psPass::Defer(psRenderable* r, const psParent& parent)
{
  _defer.AddConstruct(r, parent);
}

void psPass::_sort(psRenderable* r)
{
  if(!r->_psort) r->_psort = _renderlist.Insert(r);
  r->_internalflags |= psRenderable::INTERNALFLAG_ACTIVE;
}