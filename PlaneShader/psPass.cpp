// Copyright ©2016 Black Sphere Studios
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

psPass::psPass(psMonitor* monitor) : _cam(&psCamera::default_camera), _renderables(0), _defaultrt(0), _renderlist(&_renderalloc), _cullgroups(0), _clear(false),
  _clearcolor(0), _dpi(0), _monitor(monitor)
{

}
psPass::~psPass()
{
  while(_renderables) _renderables->SetPass(0);
  while(_cullgroups) _cullgroups->SetPass(0);
}

void psPass::Begin()
{
  PROFILE_FUNC();
  CurPass = this;
  _driver->SetDPIScale(psVec(GetDPI()/(float)psGUIManager::BASE_DPI));
  auto rt = GetRenderTarget();
  if(!rt) return;

  const psRect& window = _cam->Apply(*rt);

  if(_clear)
    _driver->Clear(_clearcolor);

  // We go through all the renderables and solids when the pass begins because we've already applied
  // the camera, and this allows you to do proper post-processing with immediate render commands.
  for(psRenderable* cur = _renderables; cur != 0; cur = cur->_llist.next)
    cur->Render();

  // Then we go through any specialized culling groups
  for(psCullGroup* group = _cullgroups; group != 0; group=group->next)
    group->Traverse(window._ltrbarray, _cam->GetPosition().z);

  // Go through our sorted list of renderables and render them all.
  auto node = _renderlist.Front(); 
  auto next = node;
  while(node) {
    if(!(node->value->_internalflags&psRenderable::INTERNALFLAG_ACTIVE)) {
      next = node->next;
      node->value->_psort = 0;
      _renderlist.Remove(node);
      node = next;
    } else {
      node->value->_render();
      node->value->_internalflags &= ~psRenderable::INTERNALFLAG_ACTIVE;
      node = node->next;
    }
  }
}

void psPass::End()
{
  _driver->Flush();
  CurPass = 0;
  _driver->PopCamera();
  PROFILE_FUNC();
}
uint32_t psPass::GetDPI()
{
  if(!_dpi)
    return !_monitor ? psEngine::Instance()->GetGUI().dpi : _monitor->dpi;
  return _dpi;
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
    AltLLRemove<psRenderable, &psPass::GetRenderableAlt>(r, _renderables);
    r->_llist.prev = r->_llist.next = 0;
    if(r->_psort != 0) _renderlist.Remove(r->_psort);
    r->_psort = 0;
  }
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