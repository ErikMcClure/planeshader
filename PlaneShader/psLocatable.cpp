// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psLocatable.h"
#include "bss-util/delegate.h"

using namespace planeshader;

psLocatable::psLocatable(const psLocatable& copy) : _relpos(copy._relpos), _rotation(copy._rotation), _pivot(copy._pivot) {}
psLocatable::psLocatable(const DEF_LOCATABLE& def) : _relpos(def.pos), _rotation(def.rotation), _pivot(def.pivot) {}
psLocatable::psLocatable(const psVec3D& pos, FNUM rotation, const psVec& pivot) : _relpos(pos), _rotation(rotation), _pivot(pivot) {}
psLocatable::~psLocatable() {}
void BSS_FASTCALL psLocatable::SetRotation(FNUM rotation) { _rotation=rotation; }
void BSS_FASTCALL psLocatable::SetPivot(const psVec& pivot) { _pivot=pivot; }
void BSS_FASTCALL psLocatable::SetPosition(FNUM X, FNUM Y, FNUM Z) { _relpos.x=X; _relpos.y=Y; _relpos.z=Z; }

void psLocatable::TypeIDRegFunc(bss_util::AniAttribute* p)
{
  /*switch(p->typeID)
  {
  case POSITION_ANI_TYPEID:
    p->Attach(&bss_util::AttrDefSmooth<POSITION_ANI_TYPEID>(&_relpos, bss_util::delegate<void, const psVec&>::From<psLocatable, &psLocatable::SetPosition>(this)));
    break;
  case ROTATION_ANI_TYPEID:
    p->Attach(&bss_util::AttrDefSmooth<ROTATION_ANI_TYPEID>(&_rotation, bss_util::delegate<void, FNUM>::From<psLocatable, &psLocatable::SetRotation>(this)));
    break;
  case PIVOT_ANI_TYPEID:
    p->Attach(&bss_util::AttrDefSmooth<PIVOT_ANI_TYPEID>(&_pivot, bss_util::delegate<void, const psVec&>::From<psLocatable, &psLocatable::SetPivot>(this)));
    break;
  }*/
}