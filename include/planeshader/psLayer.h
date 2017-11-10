// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#ifndef __LAYER_H__PS__
#define __LAYER_H__PS__

#include "psRenderable.h"
#include "psCamera.h"
#include "psTex.h"
#include "bss-util/BlockAllocMT.h"
#include "bss-util/Stack.h"

namespace planeshader {
  class psSolid;

  // Represents a render surface, which can be a backbuffer, texture, or multiple textures
  class PS_DLLEXPORT psLayer : public psRenderable, public psDriverHold
  {
  public:
    psLayer(psLayer&& mov);
    psLayer();
    psLayer(psTex* const* targets, uint8_t num);
    inline explicit psLayer(psTex* target) : psLayer(&target, 1) {}
    template<int N>
    inline psLayer(psTex* (&targets)[N]) : psLayer(targets, N) { }
    ~psLayer();
    void Push(const psTransform2D& parent);
    void Pop();
    inline void SetCamera(const psCamera* camera = 0) { _cam = const_cast<psCamera*>(camera); }
    inline const psCamera* GetCamera() const { return _cam; }
    inline psTex* const* GetTargets() const { return reinterpret_cast<psTex* const*>(_targets.begin()); }
    inline uint8_t NumTargets() const { return _targets.Capacity(); }
    inline void SetTarget(psTex* target) { SetTargets(&target, 1); }
    void SetTargets(psTex* const* targets, uint8_t num);
    inline psVeciu GetDPI() const { return _dpi; }
    inline void SetDPI(psVeciu dpi) { _dpi = dpi; }
    void Insert(psRenderable* r);
    void Remove(psRenderable* r);
    void Defer(psRenderable* r, const psTransform2D& parent);
    bool Cull(psSolid* solid, const psTransform2D* parent) const;
    inline const psCamera::Culling& GetCulling() const { assert(CurLayers.Peek() == this); return _cull; }

    inline void SetClearColor(psColor32 color, bool clear = true) { _clearcolor = color; _clear = clear; }
    inline psColor32 GetClearColor() const { return _clearcolor; }

    psLayer& operator=(psLayer&& mov);
    static psLayer* CurLayer();
    static psCamera* CurCamera();

    typedef bss::TRB_Node<std::pair<psRenderable*, const psTransform2D*>> NODE;
    friend class psRenderable;
    friend class psCullGroup;

  protected:
    void _sort(psRenderable* r, const psTransform2D& t);
    static void _applytop();
    virtual void _render(const psTransform2D& parent) override;

    bss::ref_ptr<psCamera> _cam;
    psCamera::Culling _cull;
    psRenderable* _renderables;
    psVeciu _dpi;
    psColor32 _clearcolor;
    bool _clear;
    bss::DynArray<std::pair<psRenderable*, psTransform2D>> _defer;
    bss::TRBtree<std::pair<psRenderable*, const psTransform2D*>, bss::CompTFirst<psRenderable*, const psTransform2D*, psRenderable::StandardCompare>, bss::PolymorphicAllocator<NODE, bss::LocklessBlockPolicy>> _renderlist;
    bss::Array<bss::ref_ptr<psTex>, uint8_t, bss::ARRAY_CONSTRUCT> _targets;

    static bss::Stack<psLayer*> CurLayers;
  };
}

#endif