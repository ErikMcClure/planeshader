// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psVector.h"
#include "bss-util\bss_algo.h"

using namespace planeshader;
using namespace bss_util;

psQuadraticHull::psQuadraticHull() : psRenderable(0, 0, 0, _driver->library.CURVE, 0, INTERNALTYPE_CURVE) {}
psQuadraticHull::~psQuadraticHull() {}

void BSS_FASTCALL psQuadraticHull::_render(psBatchObj* obj)
{
  if(obj)
  {
    if((obj->buffer->nvert + _verts.Length()) >= CURVEBUFSIZE)
    {
      _driver->DrawBatchEnd(*obj);
      obj->mem = _driver->LockBuffer(obj->buffer->verts, LOCK_WRITE_DISCARD);
    }

    memcpy(obj->mem, _verts.begin(), sizeof(QuadVertex)*_verts.Length());
    obj->buffer->nvert += _verts.Length();
  }
  else
  {
    psRenderable* r = this;
    _renderbatch(&r, 1);
  }
}

void BSS_FASTCALL psQuadraticHull::_renderbatch(psRenderable** rlist, unsigned int count)
{
  static psVertObj vertobj = { _driver->CreateBuffer(sizeof(QuadVertex)*CURVEBUFSIZE, USAGE_VERTEX | USAGE_DYNAMIC, 0), 0, 0, 0, sizeof(QuadVertex), FMT_INDEX16, TRIANGLELIST };

  Activate(this);
  psBatchObj obj;
  _driver->DrawBatchBegin(obj, GetAllFlags(), &vertobj);

  for(size_t i = 0; i < count; ++i)
    rlist[i]->_render(&obj);

  _driver->DrawBatchEnd(obj);
}

void psQuadraticHull::Clear()
{
  _verts.Clear();
}

void BSS_FASTCALL psQuadraticHull::AppendQuadraticCurve(psVec p0, psVec p1, psVec p2, float thickness, unsigned int color)
{
  psVec p[3] = { p0, p1, p2 };
  psVec padjust[3] = { p0, p1, p2 };
  // To prevent degenerate triangles, calculate the geometric center and push the points thickness away from it, recalculating the center after each adjustment.
  for(int i = 0; i < 3; ++i)
  {

  }
  _verts.SetLength(_verts.Length() + 3);
  QuadVertex* v = _verts.end() - 3;
  for(int i = 0; i < 3; ++i)
  {
    v[i].x = padjust[i].x;
    v[i].y = padjust[i].y;
    v[i].z = 0;
    v[i].t = 1;
    v[i].u = padjust[i].x;
    v[i].v = padjust[i].y;
    v[i].w = 0;
    v[i].p1x = p[0].x;
    v[i].p1y = p[0].y;
    v[i].p1z = 0;
    v[i].p1t = thickness;
    v[i].p2x = p[1].x;
    v[i].p2y = p[1].y;
    v[i].p2z = 0;
    v[i].p2t = thickness;
    v[i].p3x = p[2].x;
    v[i].p3y = p[2].y;
    v[i].p3z = 0;
    v[i].p3t = thickness;
    v[i].color = color;
  }
}


psQuadraticCurve::psQuadraticCurve(psVec p0, psVec p1, psVec p2, float thickness, unsigned int color) : psColored(color), _thickness(thickness) { Set(p0, p1, p2); }
psQuadraticCurve::psQuadraticCurve(psVec(&p)[3], float thickness, unsigned int color) : psColored(color), _thickness(thickness) { Set(p); }
psQuadraticCurve::~psQuadraticCurve() {}
void psQuadraticCurve::Set(psVec p0, psVec p1, psVec p2)
{
  _p[0] = p0;
  _p[1] = p1;
  _p[2] = p2;
  Clear();
  AppendQuadraticCurve(_p[0], _p[1], _p[2], _thickness, _color.color);
}
void psQuadraticCurve::Set(psVec(&p)[3])
{
  Set(p[0], p[1], p[2]);
}

template<typename T, typename FN>
inline static void IterateCubic(const T(&P0)[2], const T(&P1)[2], const T(&P2)[2], const T(&P3)[2], FN fn, int n)
{
  T N1[2];
  T N2[2];
  T N3[2];
  T R1[2];
  T R2[2];
  T dt = 1.0 / (n + 1);

  T W0[2] = { P0[0], P0[1] };
  T W1[2] = { P1[0], P1[1] };
  T W2[2] = { P2[0], P2[1] };
  T W3[2] = { P3[0], P3[1] };
  for(int i = 0; i < n; ++i)
  {
    SplitCubic(dt * (i + 1), W0, W1, W2, W3, N1, N2, N3, R1, R2);

    T C[2] = { (N1[0] + N2[0]) / 2, (N1[1] + N2[1]) / 2 };
    fn(W0, C, N3);
    for(int i = 0; i < 2; ++i)
    {
      W0[i] = N3[i];
      W1[i] = R1[i];
      W2[i] = R2[i];
    }
  }

  T C[2] = {(W1[0] + W2[0]) / 2, (W1[1] + W2[1]) / 2 };
  fn(W0, C, W3);
}

psCubicCurve::psCubicCurve(psVec p0, psVec p1, psVec p2, psVec p3, float thickness, unsigned int color, float maxerr) : psColored(color), _thickness(thickness), _maxerr(maxerr) { Set(p0, p1, p2, p3); }
psCubicCurve::psCubicCurve(psVec(&p)[4], float thickness, unsigned int color) : psColored(color), _thickness(thickness) { Set(p); }
psCubicCurve::~psCubicCurve() {}
void psCubicCurve::Set(psVec p0, psVec p1, psVec p2, psVec p3)
{
  _p[0] = p0;
  _p[1] = p1;
  _p[2] = p2;
  _p[3] = p3;

  Clear();
  //IterateCubic<float>(p0.v, p1.v, p2.v, p3.v, delegate<void, const float(&)[2], const float(&)[2], const float(&)[2]>::From<psCubicCurve, &psCubicCurve::_addquad>(this), 20);
  ApproxCubic<float>(p0.v, p1.v, p2.v, p3.v, delegate<void, const float(&)[2], const float(&)[2], const float(&)[2]>::From<psCubicCurve, &psCubicCurve::_addquad>(this), _maxerr);
}
void psCubicCurve::Set(psVec(&p)[4])
{
  Set(p[0], p[1], p[2], p[3]);
}
void psCubicCurve::_addquad(const float(&P0)[2], const float(&P1)[2], const float(&P2)[2])
{
  AppendQuadraticCurve(psVec(P0), psVec(P1), psVec(P2), _thickness, _color.color);
}
