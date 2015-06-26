// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __RENDERABLE_H__PS__
#define __RENDERABLE_H__PS__

#include "ps_dec.h"
#include "bss-util/cBitField.h"
#include "bss-util/LLBase.h"
#include "bss-util/cTRBtree.h"

namespace planeshader {
  struct DEF_RENDERABLE;
  class psStateblock;
  class psShader;
  class psTex;

  class PS_DLLEXPORT psRenderable
  {
    friend class psPass;

  public:
    psRenderable(const psRenderable& copy);
    psRenderable(psRenderable&& mov);
    psRenderable(const DEF_RENDERABLE& def);
    explicit psRenderable(FLAG_TYPE flags=0, int zorder=0, psStateblock* stateblock=0, psShader* shader=0, unsigned short pass=(unsigned short)-1);
    virtual ~psRenderable();
    virtual void Render();
    inline int GetZOrder() const { return _zorder; }
    inline void BSS_FASTCALL SetZOrder(int zorder) { _zorder=zorder; _invalidate(); }
    inline unsigned short GetPass() const { return _pass; }
    virtual void BSS_FASTCALL SetPass(unsigned short pass);
    inline bss_util::cBitField<FLAG_TYPE>& GetFlags() { return _flags; }
    inline FLAG_TYPE GetFlags() const { return _flags; }
    inline psShader* GetShader() const { return _shader; }
    inline void BSS_FASTCALL SetShader(psShader* shader) { _shader=shader; }
    inline psStateblock* GetStateblock() const { return _stateblock; }
    void BSS_FASTCALL SetStateblock(psStateblock* stateblock);
    virtual psTex* const* GetTextures() const;
    virtual unsigned char NumTextures() const;
    virtual psTex* const* GetRenderTargets() const;
    virtual unsigned char NumRT() const;

    psRenderable& operator =(const psRenderable& right);
    psRenderable& operator =(psRenderable&& right);

  protected:
    void _destroy();
    void _invalidate();
    BSS_FORCEINLINE unsigned char _internaltype() { return _internalflags&0x3F; }
    virtual void _render()=0;
    virtual void BSS_FASTCALL _renderbatch(psRenderable** rlist);
    virtual char BSS_FASTCALL _sort(psRenderable* r) const;
    virtual bool BSS_FASTCALL _batch(psRenderable* r) const;

    bss_util::cBitField<FLAG_TYPE> _flags;
    unsigned short _pass; // Stores what pass we are in, or is set to -1 if we aren't assigned to a pass
    unsigned char _internalflags;
    int _zorder;
    psStateblock* _stateblock;
    psShader* _shader; // any shader left in here when cRenderable is destroyed will be deleted, but changing it out won't delete the shader (so you can do batch render tricks)
    bss_util::LLBase<psRenderable> _llist;
    bss_util::TRB_Node<psRenderable*>* _psort;

    enum INTERNALTYPE : unsigned char
    {
      INTERNALTYPE_NONE = 0,
      INTERNALTYPE_IMAGE,
    };

    enum INTERNALFLAGS : unsigned char
    {
      INTERNALFLAG_ACTIVE = 0x80,
      INTERNALFLAG_SOLID = 0x40,
      INTERNALFLAG_MASK = 0x3F,
    };
  };

  struct BSS_COMPILER_DLLEXPORT DEF_RENDERABLE
  {
    DEF_RENDERABLE() : flags(0), zorder(0), pass((unsigned short)-1) {}
    inline virtual psRenderable* BSS_FASTCALL Spawn() const { return 0; } //This creates a new instance of whatever class this definition defines
    inline virtual DEF_RENDERABLE* BSS_FASTCALL Clone() const { return new DEF_RENDERABLE(*this); }

    FLAG_TYPE flags;
    int zorder;
    unsigned short pass;
    psStateblock* stateblock;
    psShader* shader;
  };
}

#endif