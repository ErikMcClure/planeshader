// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#include "psEngine.h"
#include "psCullGroup.h"
#include "psLayer.h"
#include "bss-util/Delegate.h"

using namespace planeshader;

psCullGroup::psCullGroup(psCullGroup&& mov) : psRenderable(std::move(mov)), _tree(std::move(mov._tree)), _nodealloc(std::move(mov._nodealloc)),
  _list(&psEngine::Instance()->NodeAlloc)
{}
psCullGroup::psCullGroup(psFlag flags, int zorder, psStateblock* stateblock, psShader* shader, psLayer* pass) :
  psRenderable(flags, zorder, stateblock, shader, pass)
{}
psCullGroup::~psCullGroup() { SetPass(0); Clear(); }
void psCullGroup::Insert(psSolid* img, bool recalc)
{
  if(img->_layer != 0) // Ensure the image is not actually in the layer's render list, if it has a layer set
    img->_layer->Remove(img);
  img->GetBoundingRect(psTransform2D::Zero); // Make sure the bounding rect is up to date
  if(recalc)
    _tree.Insert(img);
  else
    _tree.InsertRoot(img);
}
void psCullGroup::Remove(psSolid* img) { _tree.Remove(img); }
void psCullGroup::Solve() { _tree.Solve(); }
void psCullGroup::Clear()
{
  for(auto p = _list.Front(); p != 0; p = p->next)
    p->value.first->_psort = 0;
  _tree.TraverseAll([](psSolid* p) {
    if(p->_layer) // Re-insert any renderables with a layer back into their appropriate layer or we'll lose them.
      p->_layer->Insert(p);
  });
  _tree.Clear();
}

void psCullGroup::_render(const psTransform2D& parent)
{
  psLayer* cur = psLayer::CurLayer();
  assert(cur);
  float camZ = cur->GetCulling().z;
  BSS_ALIGN(16) float rcull[4];
  AdjustRect(cur->GetCulling().full.RelativeTo(parent.position, parent.rotation, parent.pivot).BuildAABB().ltrb, camZ, rcull);

  _tree.TraverseAction(rcull, [this,cur,parent](psSolid* p) {
    if(!p->_layer)
    {
      if(!p->_psort)
        p->_psort = _list.Insert(std::pair<psRenderable*, const psTransform2D*>(p, 0));
      p->_internalflags |= psRenderable::INTERNALFLAG_ACTIVE;
    }
    else if(p->_layer == cur)
      p->_render(parent);
    else
      p->_layer->_sort(p, parent);
  });

  auto node = _list.Front(); // Render any children that did not interweave with the pass
  auto next = node;
  while(node)
  {
    if(!(node->value.first->_internalflags&psRenderable::INTERNALFLAG_ACTIVE))
    {
      next = node->next;
      node->value.first->_psort = 0;
      _list.Remove(node);
      node = next;
    }
    else
    {
      if(node->value.first->_layer == psLayer::CurLayer())
        node->value.first->_render(parent);
      else
        node->value.first->_layer->Defer(node->value.first, parent);
      node->value.first->_internalflags &= ~psRenderable::INTERNALFLAG_ACTIVE;
      node = node->next;
    }
  }
}
