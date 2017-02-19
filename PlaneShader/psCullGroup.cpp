// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psCullGroup.h"
#include "psPass.h"
#include "bss-util\delegate.h"

using namespace planeshader;

psCullGroup::psCullGroup(psCullGroup&& mov) : _tree(std::move(mov._tree)), _nodealloc(std::move(mov._nodealloc)), _pass(0)
{
  psPass* pass = mov._pass;
  mov.SetPass(0);
  SetPass(pass);
}
psCullGroup::psCullGroup() : _pass(0) {}
psCullGroup::~psCullGroup() { SetPass(0); Clear(); }
void psCullGroup::Insert(psSolid* img, bool recalc)
{ 
  img->SetPass(_pass); // Set pass to our pass
  _pass->Remove(img); // In case the pass was already set to our pass, ensure that we are NOT on the internal renderlist (this does not change img's _pass)
  img->UpdateBoundingRect(); // Make sure the bounding rect is up to date
  if(recalc)
    _tree.Insert(img);
  else
    _tree.InsertRoot(img);
}
void psCullGroup::Remove(psSolid* img) { _tree.Remove(img); }
void psCullGroup::Solve() { _tree.Solve(); }
void psCullGroup::Clear() { _tree.Clear(); }

void psCullGroup::Traverse(const float(&rect)[4], FNUM camZ)
{
  BSS_ALIGN(16) float rcull[4];
  float diff=((rect[1]+rect[3])*0.5f);
  float diff2= ((rect[0]+rect[2])*0.5f);
  sseVec d(diff2, diff, diff2, diff);
  (((sseVec(rect)-d)*sseVec(1+camZ))+d)>>rcull;
  
  _tree.Traverse(rcull);
}

void psCullGroup::SetPass(psPass* pass)
{
  if(_pass) _pass->_removecullgroup(this);
  _pass = pass;
  if(_pass) _pass->_addcullgroup(this);
}
