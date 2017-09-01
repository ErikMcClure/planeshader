// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#ifndef __RENDERABLE_H__PS__
#define __RENDERABLE_H__PS__

#include "psStateBlock.h"
#include "psShader.h"
#include "psTransform2D.h"
#include "bss-util/BitField.h"
#include "bss-util/LLBase.h"
#include "bss-util/TRBtree.h"

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
    virtual void Render(const psTransform2D* parent);
    int GetZOrder() const { return _zorder; }
    void SetZOrder(int zorder);
    inline psLayer* GetLayer() const { return _layer; }
    void SetPass(psLayer* pass);
    void SetPass(); // Sets the pass to the 0th pass.
    inline bss::BitField<psFlag>& GetFlags() { return _flags; }
    inline psFlag GetFlags() const { return _flags; }
    inline psShader* GetShader() { return _shader; }
    inline const psShader* GetShader() const { return _shader; }
    inline void SetShader(psShader* shader) { _shader=shader; }
    inline const psStateblock* GetStateblock() const { return _stateblock; }
    void SetStateblock(psStateblock* stateblock);
    virtual psTex* const* GetTextures() const;
    virtual uint8_t NumTextures() const;
    virtual psRenderable* Clone() const { return 0; }

    psRenderable& operator =(const psRenderable& right);
    psRenderable& operator =(psRenderable&& right);

    static BSS_FORCEINLINE bss::LLBase<psRenderable>& GetRenderableAlt(psRenderable* r) { return r->_llist; }
    static BSS_FORCEINLINE char StandardCompare(psRenderable* const& l, psRenderable* const& r)
    {
      char c = SGNCOMPARE(l->_zorder, r->_zorder);
      return !c ? SGNCOMPARE(l, r) : c;
    }

    void Activate();
    virtual void _render(const psTransform2D& parent) = 0;

  protected:
    void _destroy();
    void _copyinsert(const psRenderable& r);
    void _invalidate();

    bss::BitField<psFlag> _flags;
    psLayer* _layer; // Stores what pass we are in
    int _zorder;
    uint8_t _internalflags;
    bss::ref_ptr<psStateblock> _stateblock;
    bss::ref_ptr<psShader> _shader;
    bss::LLBase<psRenderable> _llist;
    bss::TRB_Node<psRenderable*>* _psort;

    enum INTERNALFLAGS : uint8_t
    {
      INTERNALFLAG_ACTIVE = 0x80,
    };
  };
}

#endif