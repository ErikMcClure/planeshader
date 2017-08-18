// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#ifndef __PASS_H__PS__
#define __PASS_H__PS__

#include "psDriver.h"
#include "psRenderable.h"
#include "psCamera.h"
#include "bss-util\BlockAlloc.h"
#include "bss-util\DynArray.h"

namespace planeshader {
  class psMonitor;

  // Defines a single, encapsulated render pass in the pipeline
  class PS_DLLEXPORT psPass : public psDriverHold
  {
    friend class psEngine;

  public:
    explicit psPass(psMonitor* monitor = 0);
    ~psPass();
    void Begin();
    void End();
    inline void SetCamera(const psCamera* camera) { _cam=!camera?&psCamera::default_camera:camera; }
    inline const psCamera* GetCamera() const { return _cam; }
    psTex* const* GetRenderTarget();
    inline void SetRenderTarget(psTex* rt=0) { _defaultrt = rt; }
    void Insert(psRenderable* r);
    void Remove(psRenderable* r);
    void Defer(psRenderable* r, const psTransform2D& parent);
    inline void SetClearColor(uint32_t color, bool enable = true) { _clearcolor = color; _clear = enable; }
    inline uint32_t GetClearColor() const { return _clearcolor; }
    psVeciu GetDPI();
    inline void SetDPI(psVeciu dpi = psVeciu(0,0)) { _dpi = dpi; }
    inline psMonitor* GetMonitor() const { return _monitor; }
    inline void SetMonitor(psMonitor* monitor = 0) { _monitor = monitor; }

    static BSS_FORCEINLINE bss::LLBase<psRenderable>& GetRenderableAlt(psRenderable* r) { return r->_llist; }
    static BSS_FORCEINLINE char StandardCompare(psRenderable* const& l, psRenderable* const& r)
    {
      char c = SGNCOMPARE(l->_zorder, r->_zorder);
      return !c ? SGNCOMPARE(l,r) : c; 
    }
    static psPass* CurPass;

    typedef bss::BlockPolicy<bss::TRB_Node<psRenderable*>> ALLOC;
    friend class psRenderable;
    friend class psCullGroup;

  protected:
    void _sort(psRenderable* r);

    const psCamera* _cam;
    psRenderable* _renderables;
    psTex* _defaultrt;
    ALLOC _renderalloc;
    uint32_t _clearcolor;
    bool _clear;
    psVeciu _dpi;
    psMonitor* _monitor;
    bss::DynArray<std::pair<psRenderable*, psTransform2D>> _defer;
    bss::TRBtree<psRenderable*, StandardCompare, ALLOC> _renderlist;
  };
}

#endif