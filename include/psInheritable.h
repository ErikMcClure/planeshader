// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __INHERITABLE_H__PS__
#define __INHERITABLE_H__PS__

#include "psLocatable.h"
#include "psRenderable.h"

namespace planeshader {
  struct DEF_INHERITABLE;

  class PS_DLLEXPORT psInheritable : public psLocatable, public psRenderable
  {
  public:
    psInheritable(const psInheritable& copy);
    psInheritable(psInheritable&& mov);
    psInheritable(const DEF_INHERITABLE& def);
    explicit psInheritable(const psVec3D& position=VEC3D_ZERO, FNUM rotation=0.0f, const psVec& pivot=VEC_ZERO, FLAG_TYPE flags=0, int zorder=0, psStateblock* stateblock=0, psShader* shader=0, unsigned short pass=(unsigned short)-1, psInheritable* parent=0);
    virtual ~psInheritable();
    // Sets/gets the parent
    void SetParent(psInheritable* parent);
    inline psInheritable* GetParent() const { return _parent; }
    // Gets all the children
    inline psInheritable* GetChildren() const { return _children; }
    // Gets the rotation plus the rotation of our parents
    inline FNUM GetTotalRotation() const { return !_parent?_rotation:(_rotation+_parent->GetTotalRotation()); }
    // Gets the absolute position by adding up all the parent positions.
    inline void GetTotalPosition(psVec3D& pos) const { pos = _relpos; if(_parent!=0) _parent->_gettotalpos(pos); }
    // Gets all the flags inherited from the parent
    inline FLAG_TYPE GetTotalFlags() const { return !_parent?_flags:(_flags|(_parent->GetTotalFlags()&PSFLAG_INHERITABLE)); }
    // Overloads SetPass so it propogates to our children
    virtual void BSS_FASTCALL SetPass(unsigned short pass);
    // Clone function
    virtual psInheritable* BSS_FASTCALL Clone() const { return 0; }

    psInheritable& operator=(const psInheritable& copy);
    psInheritable& operator=(psInheritable&& mov);

    inline static bss_util::LLBase<psInheritable>& GetLLBase(psInheritable* x) { return x->_lchild; }

  protected:
    void _gettotalpos(psVec3D& pos) const;

    bss_util::LLBase<psInheritable> _lchild;
    psInheritable* _parent;
    psInheritable* _children;
  };

  struct BSS_COMPILER_DLLEXPORT DEF_INHERITABLE : DEF_LOCATABLE, DEF_RENDERABLE
  {
    inline DEF_INHERITABLE() {}
    inline virtual psInheritable* BSS_FASTCALL Spawn() const { return 0; } //This creates a new instance of whatever class this definition defines
    inline virtual DEF_INHERITABLE* BSS_FASTCALL Clone() const { return new DEF_INHERITABLE(*this); }

    psInheritable* parent;
  };
}

#endif