// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psEffect.h"
#include <algorithm>

using namespace planeshader;

psEffect::psEffect(const psVec3D& position, FNUM rotation, const psVec& pivot, psFlag flags, int zorder, psPass* pass) :
  psGroup(position, rotation, pivot, flags, zorder, 0, 0, pass)
{

}

bool psEffect::Link(psRenderable* src, uint8_t srcindex, psRenderable* dest, uint8_t destindex)
{
  Edge e = { src, srcindex, dest, destindex };
  if(destindex >= dest->NumTextures())
    return false; //invalid index

  uint32_t ind = _edges.Insert(e);
  if(!_sort())
  {
    _edges.Remove(ind); // the edge we added was invalid so remove it and return false
    _sort(); // Fix the graph
    return false;
  }

  src->SetRenderTarget(dest->GetTextures()[srcindex], destindex);
  if(destindex >= src->NumRT() || src->GetRenderTargets()[destindex] != dest->GetTextures()[srcindex])
  { 
    _edges.Remove(ind);
    _sort();
    return false; // If this happens, we weren't able to set the render target, usually because you passed in something that can't hold any.
  }

  return true;
}

// Topological sort using Tarjan's algorithm
void psEffect::_sortvisit(psRenderable* child, bool& fail, uint32_t& order)
{
  static const int CYCLEFLAG = (1 << 31);
  static const int VISITEDFLAG = (1 << 30);

  if(child->GetZOrder()&CYCLEFLAG)
    fail = true;
  if(fail) return;
  if(!(child->GetZOrder()&VISITEDFLAG))
  {
    child->SetZOrder(child->GetZOrder() | CYCLEFLAG);
    Edge val = { child, 0, 0, 0 };
    Edge* begin = std::lower_bound<Edge*>(_edges.begin(), _edges.end(), val, LessEdge);
    while(begin != _edges.end() && begin->f == child)
    {
      _sortvisit(begin->t, fail, order);
      ++begin;
    }

    assert(order > 0);
    child->SetZOrder(--order | VISITEDFLAG); // removes cycle flag, adds visited flag on top of new order
  }
}

bool psEffect::_sort()
{
  static const int FLAGMASK = (1 << 31) | (1 << 30);
  bool fail = false;
  uint32_t order = NumChildren();
  
  MapToChildren([&](psRenderable* child) { _sortvisit(child, fail, order); });
  MapToChildren([&](psRenderable* child) { child->SetZOrder(child->GetZOrder()&(~FLAGMASK)); }); // Fix all the zorders
  return !fail;
}
