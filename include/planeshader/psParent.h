// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __PARENT_H__PS__
#define __PARENT_H__PS__

#include "psDriver.h"
#include "bss-util/vector.h"

namespace planeshader {
  struct PS_DLLEXPORT psParent
  {
    psVec3D position;
    float rotation;
    psVec pivot;

    BSS_FORCEINLINE psParent Push(const psParent& p) const { return Push(p.position, p.rotation, p.pivot); }
    inline psParent Push(const psVec3D& pos, float r, const psVec& p) const { return psParent{ CalcPosition(pos, r, p), r + rotation, p }; }
    BSS_FORCEINLINE psVec3D CalcPosition(const psParent& p) const { return CalcPosition(p.position, p.rotation, p.pivot); }
    inline psVec3D CalcPosition(const psVec3D& pos, float r, const psVec& p) const
    {
      psVec3D ret(pos);
      if(r != 0.0f)
        psVec::RotatePoint(ret.x, ret.y, r, pivot.x, pivot.y);
      return ret + position;
    }
    inline void GetTransform(psMatrix& matrix) const
    {
      bss::Matrix<float, 4, 4>::AffineTransform_T(position.x - pivot.x, position.y - pivot.y, position.z, rotation, pivot.x, pivot.y, matrix);
    }
    BSS_FORCEINLINE void GetTransform(psMatrix& matrix, const psParent* parent) const
    {
      if(!parent)
        GetTransform(matrix);
      else
        (*parent).Push(position, rotation, pivot).GetTransform(matrix);
    }

    inline bool operator ==(const psParent& r) const { return position == r.position && rotation == r.rotation && pivot == r.pivot; }

    static const psParent Zero;
  };
}


#endif