// Copyright ©2018 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#ifndef __PARENT_H__PS__
#define __PARENT_H__PS__

#include "psDriver.h"
#include "bss-util/vector.h"

namespace planeshader {
  struct PS_DLLEXPORT psTransform2D
  {
    psVec3D position;
    float rotation;
    psVec pivot;

    BSS_FORCEINLINE psTransform2D Push(const psTransform2D& p) const { return Push(p.position, p.rotation, p.pivot); }
    inline psTransform2D Push(const psVec3D& pos, float r, const psVec& p) const { return psTransform2D{ CalcPosition(pos), r + rotation, p }; }
    BSS_FORCEINLINE psVec3D CalcPosition(const psTransform2D& p) const { return CalcPosition(p.position); }
    inline psVec3D CalcPosition(const psVec3D& pos) const
    {
      psVec3D ret(pos);
      if(rotation != 0.0f)
        psVec::RotatePoint(ret.x, ret.y, rotation, pivot.x, pivot.y);
      return ret + position;
    }
    inline void GetMatrix(psMatrix& matrix) const
    {
      bss::Matrix<float, 4, 4>::AffineTransform_T(position.x - pivot.x, position.y - pivot.y, position.z, rotation, pivot.x, pivot.y, matrix);
    }
    BSS_FORCEINLINE void GetMatrix(psMatrix& matrix, const psTransform2D* parent) const
    {
      if(!parent)
        GetMatrix(matrix);
      else
        (*parent).Push(position, rotation, pivot).GetMatrix(matrix);
    }

    inline bool operator ==(const psTransform2D& r) const { return position == r.position && rotation == r.rotation && pivot == r.pivot; }

    static const psTransform2D Zero;
  };
}


#endif