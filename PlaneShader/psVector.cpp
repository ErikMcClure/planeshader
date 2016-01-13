// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psVector.h"
#include "bss-util\bss_algo.h"
#include "psTex.h"

using namespace planeshader;
using namespace bss_util;

psQuadraticHull::psQuadraticHull() : psRenderable(0, 0, 0, _driver->library.CURVE, 0, INTERNALTYPE_CURVE) {}
psQuadraticHull::~psQuadraticHull() {}

void BSS_FASTCALL psQuadraticHull::_render(psBatchObj* obj)
{
  if(obj)
  {
    QuadVertex* verts = _verts.begin();
    size_t num = _verts.Length();
    for(unsigned int i = obj->buffer->nvert + num; i > CURVEBUFSIZE; i -= CURVEBUFSIZE)
    {
      memcpy(((QuadVertex*)obj->mem) + obj->buffer->nvert, verts, (CURVEBUFSIZE - obj->buffer->nvert)*sizeof(QuadVertex));
      obj->buffer->nvert = CURVEBUFSIZE;
      _driver->DrawBatchEnd(*obj);
      obj->mem = _driver->LockBuffer(obj->buffer->verts, LOCK_WRITE_DISCARD);
      num -= CURVEBUFSIZE;
      verts += CURVEBUFSIZE;
    }

    memcpy(((QuadVertex*)obj->mem) + obj->buffer->nvert, verts, num*sizeof(QuadVertex));
    obj->buffer->nvert += num;
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

void psQuadraticHull::SetVert(float (&v)[4], psVec& x, float thickness)
{
  v[0] = x.x;
  v[1] = x.y;
  v[2] = 0;
  v[3] = thickness;
}

void BSS_FASTCALL psQuadraticHull::AppendQuadraticCurve(psVec p0, psVec p1, psVec p2, float thickness, unsigned int color, char cap)
{
  const float BUFFER = 1.0;
  psVec p[3] = { p0, p1, p2 };

  for(int i = 0; i < 3; ++i) p[i] -= p0;
  float angle = atan2(-p[2].y, p[2].x);
  for(int i = 0; i < 3; ++i) p[i] = p[i].Rotate(angle, 0, 0);

  // Now that we've aligned the curve, find an axis-aligned bounding box for it. 
  psVec t = (p[0] - p[1]) / (p[0] - 2.0f*p[1] + p[2]); // Gets the roots of the derivative, one t value for both x and y.
  if(!isfinite(t.x)) t.x = 0;
  if(!isfinite(t.y)) t.y = 0;

  t = psVec(bssclamp(t.x, 0, 1), bssclamp(t.y, 0, 1)); // Clamp to [0,1]
  psVec pmax = QuadraticBezierCurve<psVec, psVec>(t, p[0], p[1], p[2]); // Simultaneously plug in t values for x and y to get our candidate maximum point.
  psVec pmin = pmax;
  if(p[0].x > pmax.x) pmax.x = p[0].x;
  if(p[0].y > pmax.y) pmax.y = p[0].y;
  if(p[2].x > pmax.x) pmax.x = p[2].x;
  if(p[2].y > pmax.y) pmax.y = p[2].y;
  if(p[0].x < pmin.x) pmin.x = p[0].x;
  if(p[0].y < pmin.y) pmin.y = p[0].y;
  if(p[2].x < pmin.x) pmin.x = p[2].x;
  if(p[2].y < pmin.y) pmin.y = p[2].y;
  pmax += psVec(thickness + BUFFER);
  pmin -= psVec(thickness + BUFFER);

  psVec rect[4] = { pmin, { pmax.x, pmin.y}, { pmin.x, pmax.y }, pmax };
    //float m = (p[1].x - p[0].x) / (p[1].y - p[0].y); // y and x are reversed here to give us a perpendicular line.
    //float b = (p[1].y*p[0].x - p[0].y*p[1].x) / (p[1].y - p[0].y);
    //rect[0].x = std::min((rect[0].y - b) / m, rect[0].x + thickness + 1);
    //rect[2].x = std::min((rect[2].y - b) / m, rect[0].x + thickness + 1);
    //m = (p[2].x - p[1].x) / (p[2].y - p[1].y);
    //b = (p[2].y*p[1].x - p[1].y*p[2].x) / (p[2].y - p[1].y);
    //rect[1].x = std::max((rect[1].y - b) / m, rect[1].x - thickness - 1);
    //rect[3].x = std::max((rect[3].y - b) / m, rect[3].x - thickness - 1);

  for(int i = 0; i < 4; ++i) rect[i] = rect[i].Rotate(-angle, 0, 0);
  for(int i = 0; i < 4; ++i) rect[i] += p0;

  static const int VSIZE = 6;
  psVec verts[VSIZE] = { rect[0], rect[1], rect[2], rect[2], rect[1], rect[3] };

  _verts.SetLength(_verts.Length() + VSIZE);
  QuadVertex* v = _verts.end() - VSIZE;
  for(int i = 0; i < VSIZE; ++i)
  {
    v[i].x = verts[i].x;
    v[i].y = verts[i].y;
    v[i].z = 0;
    v[i].t = 1;
    SetVert(v[i].p0, p0, thickness * ((cap & 1) ? 1 : -1));
    SetVert(v[i].p1, p1, thickness);
    SetVert(v[i].p2, p2, thickness * ((cap & 2) ? 1 : -1));
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
  AppendQuadraticCurve(_p[0], _p[1], _p[2], _thickness, _color.color, 3);
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
  AppendQuadraticCurve(psVec(P0), psVec(P1), psVec(P2), _thickness, _color.color, !_verts.Length() | (psVec(P2) == _p[3])*2);
}

psRoundedRect::psRoundedRect(const psRoundedRect& copy) : psSolid(copy), psColored(copy) {}
psRoundedRect::psRoundedRect(psRoundedRect&& mov) : psSolid(std::move(mov)), psColored(std::move(mov)) {}
psRoundedRect::psRoundedRect(const psRectRotateZ& rect, psFlag flags, int zorder, psStateblock* stateblock, psShader* shader, psPass* pass, psInheritable* parent, const psVec& scale) :
  psSolid(psVec3D(rect.left, rect.top, rect.z), rect.rotation, rect.pivot, flags, zorder, stateblock, !shader?_driver->library.ROUNDRECT:shader, pass, parent, scale, INTERNALTYPE_NONE),
  _outline(0), _edge(-1), _corners(0,0,0,0)
{
  SetDim(rect.GetDimensions());
}
psRoundedRect::~psRoundedRect(){}

void BSS_FASTCALL psRoundedRect::_render(psBatchObj* obj)
{
  if(obj)
  {
    if(obj->buffer->nvert >= RRBUFSIZE)
    {
      _driver->DrawBatchEnd(*obj);
      obj->mem = _driver->LockBuffer(obj->buffer->verts, LOCK_WRITE_DISCARD);
    }

    unsigned int color;
    unsigned int outline;
    _color.WriteFormat(FMT_R8G8B8A8, &color);
    _outline.WriteFormat(FMT_R8G8B8A8, &outline);

    RRVertex* vert = (RRVertex*)obj->mem;
    const psRectRotateZ& rect = GetCollisionRect();
    *vert = { rect.left, rect.top, rect.z, rect.rotation,
      { rect.right - rect.left, rect.bottom - rect.top, rect.pivot.x, rect.pivot.y },
      _corners, _edge, color, outline };
    ++obj->buffer->nvert;
  }
  else
  {
    psRenderable* r = this;
    _renderbatch(&r, 1);
  }
}
void BSS_FASTCALL psRoundedRect::_renderbatch(psRenderable** rlist, unsigned int count)
{
  static psVertObj vertobj = { _driver->CreateBuffer(sizeof(RRVertex)*RRBUFSIZE, USAGE_VERTEX | USAGE_DYNAMIC, 0), 0, 0, 0, sizeof(RRVertex), FMT_INDEX16, POINTLIST };

  Activate(this);
  psBatchObj obj;
  _driver->DrawBatchBegin(obj, GetAllFlags(), &vertobj);

  for(size_t i = 0; i < count; ++i)
    rlist[i]->_render(&obj);

  _driver->DrawBatchEnd(obj);
}


psRenderCircle::psRenderCircle(const psRenderCircle& copy) : psSolid(copy), psColored(copy) {}
psRenderCircle::psRenderCircle(psRenderCircle&& mov) : psSolid(std::move(mov)), psColored(std::move(mov)) {}
psRenderCircle::psRenderCircle(float radius, const psVec3D& position, psFlag flags, int zorder, psStateblock* stateblock, psShader* shader, psPass* pass, psInheritable* parent, const psVec& scale) :
  psSolid(position - psVec3D(radius, radius, 0), 0, VEC_ZERO, flags, zorder, stateblock, !shader ? _driver->library.ROUNDRECT : shader, pass, parent, scale, INTERNALTYPE_NONE),
  _outline(0), _edge(-1), _arcs(0, 0, 0, 0)
{
  SetDim(psVec(radius*2));
}
psRenderCircle::~psRenderCircle() {}

void BSS_FASTCALL psRenderCircle::_render(psBatchObj* obj)
{
  if(obj)
  {
    if(obj->buffer->nvert >= CIRCLEBUFSIZE)
    {
      _driver->DrawBatchEnd(*obj);
      obj->mem = _driver->LockBuffer(obj->buffer->verts, LOCK_WRITE_DISCARD);
    }

    unsigned int color;
    unsigned int outline;
    _color.WriteFormat(FMT_R8G8B8A8, &color);
    _outline.WriteFormat(FMT_R8G8B8A8, &outline);

    CircleVertex* vert = (CircleVertex*)obj->mem;
    const psRectRotateZ& rect = GetCollisionRect();
    *vert = { rect.left, rect.top, rect.z, rect.rotation,
    { rect.right - rect.left, rect.bottom - rect.top, rect.pivot.x, rect.pivot.y },
      _arcs, _edge, color, outline };
    ++obj->buffer->nvert;
  }
  else
  {
    psRenderable* r = this;
    _renderbatch(&r, 1);
  }
}
void BSS_FASTCALL psRenderCircle::_renderbatch(psRenderable** rlist, unsigned int count)
{
  static psVertObj vertobj = { _driver->CreateBuffer(sizeof(CircleVertex)*CIRCLEBUFSIZE, USAGE_VERTEX | USAGE_DYNAMIC, 0), 0, 0, 0, sizeof(CircleVertex), FMT_INDEX16, POINTLIST };

  Activate(this);
  psBatchObj obj;
  _driver->DrawBatchBegin(obj, GetAllFlags(), &vertobj);

  for(size_t i = 0; i < count; ++i)
    rlist[i]->_render(&obj);

  _driver->DrawBatchEnd(obj);
}