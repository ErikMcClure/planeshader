// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psPass.h"
#include "psRenderable.h"

using namespace planeshader;
using namespace bss_util;


psPass::psPass() : _cam(&psCamera::default_camera), _renderables(0)
{

}
psPass::~psPass()
{

}
void psPass::Begin()
{
  auto& vp = _cam->GetViewPort();
  psRectiu realvp = { (unsigned int)bss_util::fFastRound(vp.left*_driver->screendim.x), (unsigned int)bss_util::fFastRound(vp.top*_driver->screendim.y), (unsigned int)bss_util::fFastRound(vp.right*_driver->screendim.x), (unsigned int)bss_util::fFastRound(vp.bottom*_driver->screendim.y) };
  _driver->ApplyCamera(_cam->GetPosition(), _cam->GetPivot(), _cam->GetRotation(), realvp);
}
void psPass::End()
{
  psRenderable* cur = _renderables;
  while(cur)
  {
    cur->_render();
    cur=cur->_llist.next;
  }

  //Traverse kd-tree forest
}
void psPass::Insert(psRenderable* r)
{
  //if(r->_internalflags&1)
    // it's a solid, insert to kd-tree
  //else
  AltLLAdd<psRenderable, &psPass::GetRenderableAlt>(r, _renderables);
}
void psPass::Remove(psRenderable* r)
{
  //if(r->_internalflags&1)
  // it's a solid, remove from kd-tree
  //else
  AltLLRemove<psRenderable, &psPass::GetRenderableAlt>(r, _renderables);
}
