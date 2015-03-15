// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psPass.h"
#include "psRenderable.h"
#include "bss-util/profiler.h"

using namespace planeshader;
using namespace bss_util;


psPass::psPass() : _cam(&psCamera::default_camera), _renderables(0), _defaultrt(0)
{

}
psPass::~psPass()
{

}
void psPass::Begin()
{
  PROFILE_FUNC();
  auto& vp = _cam->GetViewPort();
  psRectiu realvp = { (unsigned int)bss_util::fFastRound(vp.left*_driver->screendim.x), (unsigned int)bss_util::fFastRound(vp.top*_driver->screendim.y), (unsigned int)bss_util::fFastRound(vp.right*_driver->screendim.x), (unsigned int)bss_util::fFastRound(vp.bottom*_driver->screendim.y) };
  _driver->ApplyCamera(_cam->GetPosition(), _cam->GetPivot(), _cam->GetRotation(), realvp);
  _driver->SetDefaultRenderTarget(_defaultrt);

  // We go through all the renderables and solids when the pass begins because we've already applied
  // the camera, and this allows you to do proper post-processing with immediate render commands.
  psRenderable* cur = _renderables;
  while(cur)
  {
    cur->Render();
    cur=cur->_llist.next;
  }

  //Traverse kd-tree forest
}
void psPass::End()
{
  PROFILE_FUNC();
}
void psPass::Insert(psRenderable* r)
{
  PROFILE_FUNC();
  //if(r->_internalflags&1)
    // it's a solid, insert to kd-tree
  //else
  AltLLAdd<psRenderable, &psPass::GetRenderableAlt>(r, _renderables);
}
void psPass::Remove(psRenderable* r)
{
  PROFILE_FUNC();
  //if(r->_internalflags&1)
  // it's a solid, remove from kd-tree
  //else
  AltLLRemove<psRenderable, &psPass::GetRenderableAlt>(r, _renderables);
}
