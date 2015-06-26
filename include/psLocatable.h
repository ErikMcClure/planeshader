// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __LOCATABLE_H__PS__
#define __LOCATABLE_H__PS__

#include "psVec.h"
#include "ps_ani.h"

namespace planeshader {
  struct DEF_LOCATABLE;

  // This holds position information. 
  class PS_DLLEXPORT psLocatable : public AbstractAni
  {
  public:
    psLocatable(const psLocatable& copy);
    psLocatable(const DEF_LOCATABLE& def);
    explicit psLocatable(const psVec3D& position, FNUM rotation=0.0f, const psVec& pivot=VEC_ZERO);
    virtual ~psLocatable();
    // Gets the rotation 
    inline FNUM GetRotation() const { return _rotation; }
    // Sets the rotation of this object 
    virtual void BSS_FASTCALL SetRotation(FNUM rotation);
    // Gets the pivot
    inline const psVec& GetPivot() const { return _pivot; }
    // Sets the pivot (this is where the image will rotate around) 
    virtual void BSS_FASTCALL SetPivot(const psVec& pivot);
    // Gets the position relative to the object parent 
    inline const psVec3D& GetPosition() const { return _relpos; }
    // Sets the position of this object 
    inline void BSS_FASTCALL SetPositionX(FNUM X) { SetPosition(X, _relpos.y, _relpos.z); }
    inline void BSS_FASTCALL SetPositionY(FNUM Y) { SetPosition(_relpos.x, Y, _relpos.z); }
    inline void BSS_FASTCALL SetPositionZ(FNUM Z) { SetPosition(_relpos.x, _relpos.y, Z); }
    inline void BSS_FASTCALL SetPosition(const psVec& pos) { SetPosition(pos.x, pos.y, _relpos.z); }
    inline void BSS_FASTCALL SetPosition(const psVec3D& pos) { SetPosition(pos.x, pos.y, pos.z); }
    inline void BSS_FASTCALL SetPosition(FNUM X, FNUM Y) { SetPosition(X, Y, _relpos.z); }
    virtual void BSS_FASTCALL SetPosition(FNUM X, FNUM Y, FNUM Z);

    inline psLocatable& operator=(const psLocatable& right) { _relpos=right._relpos; _rotation=right._rotation; _pivot=right._pivot; return *this; }

  protected:
    virtual void BSS_FASTCALL TypeIDRegFunc(bss_util::AniAttribute*);

    psVec3D _relpos;
    psVec _pivot; // Rotational center in absolute coordinates relative from its location
    FNUM _rotation; //Rotation of the object
  };

  struct BSS_COMPILER_DLLEXPORT DEF_LOCATABLE
  {
    inline DEF_LOCATABLE() : pos(0,0,0), pivot(0,0), rotation(0) {}
    inline virtual psLocatable* BSS_FASTCALL Spawn() const { return new psLocatable(*this); } //This creates a new instance of whatever class this definition defines
    inline virtual DEF_LOCATABLE* BSS_FASTCALL Clone() const { return new DEF_LOCATABLE(*this); }

    psVec3D pos;
    psVec pivot;
    FNUM rotation;
  };
}
namespace bss_util { template<typename Alloc> struct AniAttributeT<planeshader::POSITION_ANI_TYPEID, Alloc> : public AniAttributeSmooth<Alloc, planeshader::psVec, planeshader::POSITION_ANI_TYPEID>{ }; }
namespace bss_util { template<typename Alloc> struct AniAttributeT<planeshader::ROTATION_ANI_TYPEID, Alloc> : public AniAttributeSmooth<Alloc, float, planeshader::ROTATION_ANI_TYPEID>{}; }
namespace bss_util { template<typename Alloc> struct AniAttributeT<planeshader::PIVOT_ANI_TYPEID, Alloc> : public AniAttributeSmooth<Alloc, planeshader::psVec, planeshader::PIVOT_ANI_TYPEID>{ }; }

#endif