// Copyright ©2018 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#ifndef __CULL_GROUP_H__PS__
#define __CULL_GROUP_H__PS__

#include "bss-util/BlockAlloc.h"
#include "bss-util/KDTree.h"
#include "bss-util/LLBase.h"
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
    BSS_FORCEINLINE uint32_t GetRBThreshold() const { return _tree.GetRBThreshold(); }
    BSS_FORCEINLINE void SetRBThreshold(uint32_t rbthreshold) { _tree.SetRBThreshold(rbthreshold); }

    typedef bss::KDNode<psSolid> KDNODE;

  protected:
    BSS_FORCEINLINE static const float* CF_FRECT(psSolid* p) { return p->GetBoundingRectStatic().ltrb; }
    BSS_FORCEINLINE static bss::LLBase<psSolid>& CF_FLIST(psSolid* p) { return *((bss::LLBase<psSolid>*)&p->_llist); }
    BSS_FORCEINLINE static KDNODE*& CF_FNODE(psSolid* p) { return p->_kdnode; }
    BSS_FORCEINLINE static void AdjustRect(const float(&rect)[4], float camZ, float(&rcull)[4])
    {
      float diff = ((rect[1] + rect[3])*0.5f);
      float diff2 = ((rect[0] + rect[2])*0.5f);
      bss::sseVec d(diff2, diff, diff2, diff);
      (((bss::sseVec(rect) - d)*bss::sseVec(1 + camZ)) + d).Set(rcull);
    }
    virtual void _render(const psTransform2D& parent) override;

    bss::KDTree<psSolid, CF_FRECT, CF_FLIST, CF_FNODE, bss::PolymorphicAllocator<KDNODE, bss::BlockPolicy>> _tree;
    bss::TRBtree<std::pair<psRenderable*, const psTransform2D*>, bss::CompTFirst<psRenderable*, const psTransform2D*,psRenderable::StandardCompare>, bss::PolymorphicAllocator<psLayer::NODE, bss::LocklessBlockPolicy>> _list;
    bss::BlockPolicy<KDNODE> _nodealloc;
  };
}

#endif