// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __PASS_H__PS__
#define __PASS_H__PS__

#include "psDriver.h"
#include "psRenderable.h"
#include "psCamera.h"
#include "bss-util\bss_alloc_block.h"
#include "bss-util\cDynArray.h"

namespace planeshader {
  class psSolid;

  // Defines a single, encapsulated render pass in the pipeline
  class PS_DLLEXPORT psPass : public psDriverHold
  {
  public:
    psPass();
    ~psPass();
    void Begin();
    void End();
    inline void BSS_FASTCALL SetCamera(const psCamera* camera) { _cam=!camera?&psCamera::default_camera:camera; }
    inline const psCamera* GetCamera() const { return _cam; }
    inline const psTex* GetDefaultRenderTarget() const { return _defaultrt; }
    inline void SetDefaultRenderTarget(const psTex* rt=0) { _defaultrt = rt; }
    void Insert(psRenderable* r);
    void Remove(psRenderable* r);
    void FlushQueue();

    static BSS_FORCEINLINE bss_util::LLBase<psRenderable>& GetRenderableAlt(psRenderable* r) { return r->_llist; }
    static BSS_FORCEINLINE char StandardCompare(psRenderable* const& l, psRenderable* const& r) { char c = SGNCOMPARE(l->_zorder, r->_zorder); if(!c) c = SGNCOMPARE(l->_internaltype(), r->_internaltype()); if(!c) c = SGNCOMPARE(l,r); return c; } //if(!c) return l->_sort(r); return c; }
    static psPass* CurPass;

    typedef bss_util::BlockPolicy<bss_util::TRB_Node<psRenderable*>> ALLOC;
    friend class psRenderable;
    friend class psCullGroup;

  protected:
    void _queue(psRenderable* r);
    void _sort(psRenderable* r);
    void _addcullgroup(psCullGroup* g);
    void _removecullgroup(psCullGroup* g);

    const psCamera* _cam;
    psRenderable* _renderables;
    psRenderable* _solids;
    psCullGroup* _cullgroups;
    const psTex* _defaultrt;
    ALLOC _renderalloc;
    bss_util::cTRBtree<psRenderable*, StandardCompare, ALLOC> _renderlist;
    bss_util::cDynArray<psRenderable*> _renderqueue;
  };
}

#endif