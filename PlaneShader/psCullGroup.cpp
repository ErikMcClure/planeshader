// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psCullGroup.h"
#include "psPass.h"

using namespace planeshader;

psCullGroup::psCullGroup(psCullGroup&& mov) : _tree(std::move(mov._tree)), _nodealloc(std::move(mov._nodealloc)), _pass(0)
{
  psPass* pass = mov._pass;
  mov.SetPass(0);
  SetPass(pass);
}
psCullGroup::psCullGroup() : _pass(0) {}
psCullGroup::~psCullGroup() { SetPass(0); Clear(); }
void BSS_FASTCALL psCullGroup::Insert(psSolid* img, bool recalc) { if(recalc) _tree.Insert(img); else _tree.InsertRoot(img); }
void BSS_FASTCALL psCullGroup::Remove(psSolid* img) { _tree.Remove(img); }
void psCullGroup::Solve() { _tree.Solve(); }
void psCullGroup::Clear() { _tree.Clear(); }

void BSS_FASTCALL psCullGroup::Traverse(const float(&rect)[4], FNUM camZ)
{
  BSS_ALIGN(16) float rcull[4];
  float diff=((rect[1]+rect[3])*0.5f);
  float diff2= ((rect[0]+rect[2])*0.5f);
  sseVec d(diff2, diff, diff2, diff);
  (((sseVec(rect)-d)*sseVec(1+camZ))+d)>>rcull;
  
  _tree.TraverseAction<bss_util::delegate<void, psSolid*>>(rcull, bss_util::delegate<void, psSolid*>::From<psCullGroup, &psCullGroup::_callsort>(this));
}
void BSS_FASTCALL psCullGroup::_callsort(psSolid* s)
{
  _pass->_sort(s);
}

void psCullGroup::SetPass(psPass* pass)
{
  if(_pass) _pass->_removecullgroup(this);
  _pass = pass;
  if(_pass) _pass->_addcullgroup(this);
}
