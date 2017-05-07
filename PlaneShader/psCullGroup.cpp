// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psCullGroup.h"
#include "psPass.h"
#include "bss-util\Delegate.h"

using namespace planeshader;

psCullGroup::psCullGroup(psCullGroup&& mov) : psRenderable(std::move(mov)), _tree(std::move(mov._tree)), _nodealloc(std::move(mov._nodealloc)),
  _list(&_alloc), _alloc(std::move(mov._alloc))
{}
psCullGroup::psCullGroup(psFlag flags, int zorder, psStateblock* stateblock, psShader* shader, psPass* pass) :
  psRenderable(flags, zorder, stateblock, shader, pass)
{}
psCullGroup::~psCullGroup() { SetPass(0); Clear(); }
void psCullGroup::Insert(psSolid* img, bool recalc)
{ 
  img->SetPass(_pass); // Set pass to our pass
  if(_pass != 0)
    _pass->Remove(img); // In case the pass was already set to our pass, ensure that we are NOT on the internal renderlist (this does not change img's _pass)
  img->GetBoundingRect(psParent::Zero); // Make sure the bounding rect is up to date
  if(recalc)
    _tree.Insert(img);
  else
    _tree.InsertRoot(img);
}
void psCullGroup::Remove(psSolid* img) { _tree.Remove(img); }
void psCullGroup::Solve() { _tree.Solve(); }
void psCullGroup::Clear() { _tree.Clear(); }

void psCullGroup::Render(const psParent* parent)
{
  psPass* pass = !_pass ? psPass::CurPass : _pass;
  if(!pass)
    return;

  float camZ = pass->GetCamera()->GetPosition().z;
  BSS_ALIGN(16) float rcull[4];
  if(!parent)
    AdjustRect(pass->GetCamera()->GetCache().window._ltrbarray, camZ, rcull);
  else
    AdjustRect(pass->GetCamera()->GetCache().full.RelativeTo(parent->position, parent->rotation, parent->pivot).BuildAABB()._ltrbarray, camZ, rcull);

  if(!parent)
  {
    _list.Clear();
    _tree.Traverse<CF_MERGE>(rcull);
  }
  else
  {
    _tree.TraverseAction(rcull, [this](psSolid* p) {
      if(!p->_psort) p->_psort = _list.Insert(p);
      p->_internalflags |= psRenderable::INTERNALFLAG_ACTIVE;
    });
    psRenderable::Render(parent);
  }
}
void psCullGroup::_render(const psParent& parent)
{
  auto node = _list.Front(); // If this is nonzero then we didn't interweave with a pass, so we manually render our children
  auto next = node;
  while(node)
  {
    if(!(node->value->_internalflags&psRenderable::INTERNALFLAG_ACTIVE))
    {
      next = node->next;
      node->value->_psort = 0;
      _list.Remove(node);
      node = next;
    }
    else
    {
      if(node->value->_pass == psPass::CurPass)
        node->value->_render(parent);
      else
        node->value->_pass->Defer(node->value, parent);
      node->value->_internalflags &= ~psRenderable::INTERNALFLAG_ACTIVE;
      node = node->next;
    }
  }
}
