// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#include "psCullGroup.h"
#include "psLayer.h"
#include "bss-util\Delegate.h"

using namespace planeshader;

psCullGroup::psCullGroup(psCullGroup&& mov) : psRenderable(std::move(mov)), _tree(std::move(mov._tree)), _nodealloc(std::move(mov._nodealloc)),
  _list(&_alloc), _alloc(std::move(mov._alloc))
{}
psCullGroup::psCullGroup(psFlag flags, int zorder, psStateblock* stateblock, psShader* shader, psLayer* pass) :
  psRenderable(flags, zorder, stateblock, shader, pass)
{}
psCullGroup::~psCullGroup() { SetPass(0); Clear(); }
void psCullGroup::Insert(psSolid* img, bool recalc)
{ 
  img->SetPass(_layer); // Set pass to our pass
  if(_layer != 0)
    _layer->Remove(img); // In case the pass was already set to our pass, ensure that we are NOT on the internal renderlist (this does not change img's _layer)
  img->GetBoundingRect(psTransform2D::Zero); // Make sure the bounding rect is up to date
  if(recalc)
    _tree.Insert(img);
  else
    _tree.InsertRoot(img);
}
void psCullGroup::Remove(psSolid* img) { _tree.Remove(img); }
void psCullGroup::Solve() { _tree.Solve(); }
void psCullGroup::Clear() { _tree.Clear(); }

void psCullGroup::Render(const psTransform2D* parent)
{
  psLayer* layer = !_layer ? psLayer::CurLayer() : _layer;
  if(!layer)
    return;

  float camZ = layer->GetCulling().z;
  BSS_ALIGN(16) float rcull[4];
  if(!parent)
    AdjustRect(layer->GetCulling().window.ltrb, camZ, rcull);
  else
    AdjustRect(layer->GetCulling().full.RelativeTo(parent->position, parent->rotation, parent->pivot).BuildAABB().ltrb, camZ, rcull);

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
void psCullGroup::_render(const psTransform2D& parent)
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
      if(node->value->_layer == psLayer::CurLayer())
        node->value->_render(parent);
      else
        node->value->_layer->Defer(node->value, parent);
      node->value->_internalflags &= ~psRenderable::INTERNALFLAG_ACTIVE;
      node = node->next;
    }
  }
}
