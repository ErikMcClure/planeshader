// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __VECTOR_H__PS__
#define __VECTOR_H__PS__

#include "psRenderable.h"
#include "psColored.h"
#include "bss-util\cDynArray.h"

namespace planeshader {
  // Used by all curve objects to render a curve as a deconstructed quadratic curve
  class PS_DLLEXPORT psQuadraticHull : public psRenderable, public psDriverHold
  {
    struct QuadVertex
    {
      float x, y, z, t;
      float u, v, w;
      float p1x, p1y, p1z, p1t;
      float p2x, p2y, p2z, p2t;
      float p3x, p3y, p3z, p3t;
      unsigned int color;
    };

  public:
    psQuadraticHull();
    ~psQuadraticHull();
    void Clear();
    void BSS_FASTCALL AppendQuadraticCurve(psVec p0, psVec p1, psVec p2, float thickness, unsigned int color);

    static const int CURVEBUFSIZE = 512 * 3;

  protected:
    virtual void BSS_FASTCALL _render(psBatchObj* obj);
    virtual void BSS_FASTCALL _renderbatch(psRenderable** rlist, unsigned int count);
    bss_util::cDynArray<QuadVertex> _verts;
  };

  class PS_DLLEXPORT psQuadraticCurve : public psQuadraticHull, public psColored
  {
  public:
    psQuadraticCurve(psVec p0, psVec p1, psVec p2, float thickness = 1.0f, unsigned int color = 0xFFFFFFFF);
    psQuadraticCurve(psVec(&p)[3], float thickness = 1.0f, unsigned int color = 0xFFFFFFFF);
    ~psQuadraticCurve();
    void Set(psVec p0, psVec p1, psVec p2);
    void Set(psVec (&p)[3]);
    inline void SetThickness(float thickness) { _thickness = thickness; }
    inline float GetThickness() const { return _thickness; }
    inline const psVec(&Get() const)[3] { return _p; }

  protected:
    psVec _p[3];
    float _thickness;
  };

  class PS_DLLEXPORT psCubicCurve : public psQuadraticHull, public psColored
  {
  public:
    psCubicCurve(psVec p0, psVec p1, psVec p2, psVec p3, float thickness = 1.0f, unsigned int color = 0xFFFFFFFF, float maxerr = 1.0f);
    psCubicCurve(psVec(&p)[4], float thickness = 1.0f, unsigned int color = 0xFFFFFFFF);
    ~psCubicCurve();
    void Set(psVec p0, psVec p1, psVec p2, psVec p3);
    void Set(psVec(&p)[4]);
    inline void SetThickness(float thickness) { _thickness = thickness; }
    inline float GetThickness() const { return _thickness; }
    inline const psVec(&Get() const)[4] { return _p; }

  protected:
    void _addquad(const float(&P0)[2], const float(&P1)[2], const float(&P2)[2]);
    psVec _p[4];
    float _thickness;
    float _maxerr;
  };

}

#endif