// Copyright (c)2026 Erik McClure
// For conditions of distribution and use, see copyright notice in ps_dec.h

#ifndef __RENDERABLE_H__PS__
#define __RENDERABLE_H__PS__

#include "psStateBlock.h"
#include "psShader.h"
#include "psTransform2D.h"
#include "buntils/BitField.h"
#include "buntils/LLBase.h"
#include "buntils/TRBtree.h"

namespace planeshader {
  class PS_DLLEXPORT psRenderable
  {
    friend class psLayer;
    friend class psCullGroup;

  public:
    psRenderable(const psRenderable& copy);
    psRenderable(psRenderable&& mov);
    explicit psRenderable(psFlag flags=0, int zorder=0, psStateblock* stateblock=0, psShader* shader=0, psLayer* pass=0);
    virtual ~psRenderable();
    virtual void Render(const psTransform2D& parent);
    int GetZOrder() const { return _zorder; }
    void SetZOrder(int zorder);
    inline psLayer* GetLayer() const { return _layer; }
    void SetPass(psLayer* pass);
    void SetPass(); // Sets the pass to the 0th pass.
    inline bun::BitField<psFlag>& GetFlags() { return _flags; }
    inline psFlag GetFlags() const { return _flags; }
    inline psShader* GetShader() { return _shader; }
    inline const psShader* GetShader() const { return _shader; }
    inline void SetShader(psShader* shader) { _shader=shader; }
    inline const psStateblock* GetStateblock() const { return _stateblock; }
    void SetStateblock(psStateblock* stateblock);
    virtual psRenderable* Clone() const { return 0; }
    virtual bool Cull(const psTransform2D& parent) { return false; }

    psRenderable& operator =(const psRenderable& right);
    psRenderable& operator =(psRenderable&& right);
    std::strong_ordering operator<=>(const psRenderable& right) const {

      auto c = _zorder <=> right._zorder;
      return c == 0 ? (this <=> &right) : c;
    }
    bool operator==(const psRenderable& right) const {
      return this == &right;
    }
    static BUN_FORCEINLINE bun::LLBase<psRenderable>& GetRenderableAlt(psRenderable* r) { return r->_llist; }

    virtual void _render(const psTransform2D& parent) = 0;

  protected:
    void _destroy();
    void _copyinsert(const psRenderable& r);
    void _invalidate();

    bun::BitField<psFlag> _flags;
    psLayer* _layer; // Stores what pass we are in
    int _zorder;
    uint8_t _internalflags;
    bun::ref_ptr<psStateblock> _stateblock;
    bun::ref_ptr<psShader> _shader;
    bun::LLBase<psRenderable> _llist;
    bun::TRB_Node<std::pair<psRenderable*, const psTransform2D*>>* _psort;

    enum INTERNALFLAGS : uint8_t
    {
      INTERNALFLAG_ACTIVE = 0x80,
    };
  };
}

#endif