// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __RENDER_GEOMETRY_H__PS__
#define __RENDER_GEOMETRY_H__PS__

#include "psLine.h"
#include "psInheritable.h"
#include "psPolygon.h"
#include "psColored.h"
#include "psDriver.h"

namespace planeshader {
  class PS_DLLEXPORT psRenderPoint : public psInheritable, public psColored, public psDriverHold
  {
  public:
    psRenderPoint(const psRenderPoint& copy);
    psRenderPoint(psRenderPoint&& mov);

    inline operator psVec3D() const { return _relpos; }
    psRenderPoint& operator =(const psRenderPoint& right);
    psRenderPoint& operator =(psRenderPoint&& right);
    inline psRenderPoint& operator =(const psVec3D& right) { SetPosition(right); return *this; }
    inline psRenderPoint& operator =(const psVec& right) { SetPosition(right); return *this; }

    static void DrawPoint(const psVec3D& p, unsigned int color);
    static void DrawPoint(const psVec& p, unsigned int color);

  protected:
    virtual void _render();
    virtual void BSS_FASTCALL _renderbatch(psRenderable** rlist, unsigned int count);
    virtual bool BSS_FASTCALL _batch(psRenderable* r) const;
  };

  class PS_DLLEXPORT psRenderLine : public psInheritable, public psColored, public psDriverHold
  {
  public:
    psRenderLine(const psRenderLine& copy);
    psRenderLine(psRenderLine&& mov);

    inline operator psLine3D() const { return psLine3D(_relpos, _point); }
    psRenderLine& operator =(const psRenderLine& right);
    psRenderLine& operator =(psRenderLine&& right);
    inline psRenderLine& operator =(const psLine3D& right) { _point = right.p2; SetPosition(right.p1); return *this; }
    inline psRenderLine& operator =(const psLine& right) { _point.xy = right.p2; SetPosition(right.p1); return *this; }

    static void DrawLine(const psLine3D& p, unsigned int color);
    static void DrawLine(const psLine& p, unsigned int color);

  protected:
    virtual void _render();
    virtual void BSS_FASTCALL _renderbatch(psRenderable** rlist, unsigned int count);
    virtual bool BSS_FASTCALL _batch(psRenderable* r) const;

  protected:
    psVec3D _point;
  };

  class PS_DLLEXPORT psRenderPolygon : public psInheritable, public psPolygon, public psColored, public psDriverHold
  {
  public:
    psRenderPolygon(const psRenderPolygon& copy);
    psRenderPolygon(psRenderPolygon&& mov);

    psRenderPolygon& operator =(const psRenderPolygon& right);
    psRenderPolygon& operator =(psRenderPolygon&& right);

    static void DrawPolygon(const psVec* p, unsigned int color);
    static void DrawPolygon(const psVertex* p, unsigned int color);

  protected:
    virtual void _render();
  };
}
#endif