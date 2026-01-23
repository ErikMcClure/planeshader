// Copyright (c)2026 Erik McClure
// For conditions of distribution and use, see copyright notice in ps_dec.h

#ifndef __CULL_GROUP_H__PS__
#define __CULL_GROUP_H__PS__

#include "buntils/BlockAlloc.h"
#include "buntils/KDTree.h"
#include "buntils/LLBase.h"
#include "psSolid.h"
#include "psLayer.h"

namespace planeshader {
  // Used to efficiently cull a group of images that are static relative to each other. Intended for static level geometry.
  class PS_DLLEXPORT psCullGroup : public psRenderable
  {
  public:
    psCullGroup(psCullGroup&& mov);
    explicit psCullGroup(psFlag flags = 0, int zorder = 0, psStateblock* stateblock = 0, psShader* shader = 0, psLayer* pass = 0);
    ~psCullGroup();
    // Inserts a solid that must not move relative to the other images in this culling group and removes it from the internal pass list
    void Insert(psSolid* img, bool recalc = false);
    // Removes a solid from this culling group and rebases the origin if necessary.
    void Remove(psSolid* img);
    // Solves the tree
    void Solve();
    // Clears the tree 
    void Clear();
    // Gets or sets the rebalance threshold
    BUN_FORCEINLINE uint32_t GetRBThreshold() const { return _tree.GetRBThreshold(); }
    BUN_FORCEINLINE void SetRBThreshold(uint32_t rbthreshold) { _tree.SetRBThreshold(rbthreshold); }

    typedef bun::KDNode<psSolid> KDNODE;

  protected:
    BUN_FORCEINLINE static const float* CF_FRECT(psSolid* p) { return p->GetBoundingRectStatic().ltrb; }
    BUN_FORCEINLINE static bun::LLBase<psSolid>& CF_FLIST(psSolid* p) { return *((bun::LLBase<psSolid>*)&p->_llist); }
    BUN_FORCEINLINE static KDNODE*& CF_FNODE(psSolid* p) { return p->_kdnode; }
    BUN_FORCEINLINE static void AdjustRect(const float(&rect)[4], float camZ, float(&rcull)[4])
    {
      float diff = ((rect[1] + rect[3])*0.5f);
      float diff2 = ((rect[0] + rect[2])*0.5f);
      bun::sseVec d(diff2, diff, diff2, diff);
      (((bun::sseVec(rect) - d)*bun::sseVec(1 + camZ)) + d).Set(rcull);
    }
    virtual void _render(const psTransform2D& parent) override;

    bun::KDTree<psSolid, CF_FRECT, CF_FLIST, CF_FNODE, bun::PolicyAllocator<KDNODE, bun::BlockPolicy>> _tree;
    bun::TRBtree<std::pair<psRenderable*, const psTransform2D*>, bun::first_three_way<psRenderable*, psRenderable*, bun::indirect_three_way>, bun::PolicyAllocator<psLayer::NODE, bun::LocklessBlockPolicy>> _list;
    bun::BlockPolicy<KDNODE> _nodealloc;
  };
}

#endif