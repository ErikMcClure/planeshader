// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#ifndef __RENDER_GEOMETRY_H__PS__
#define __RENDER_GEOMETRY_H__PS__

#include "psRect.h"
#include "psSolid.h"
#include "psColor.h"
#include "psDriver.h"
#include "psTextured.h"

namespace planeshader {
  class PS_DLLEXPORT psRenderEllipse : public psSolid, public psDriverHold
  {
  public:
    psRenderEllipse(const psRenderEllipse& copy);
    psRenderEllipse(psRenderEllipse&& mov);
    explicit psRenderEllipse(const psCircle& circle);
    explicit psRenderEllipse(const psEllipse& ellipse);
    inline psColor32 GetColor() const { return _color; }
    inline void SetColor(psColor32 color) { _color = color; }

    inline operator psEllipse() const { return psEllipse(position.x + _realdim.x, position.y + _realdim.y, _realdim.x/2, _realdim.y/2); }
    psRenderEllipse& operator =(const psRenderEllipse& right);
    psRenderEllipse& operator =(psRenderEllipse&& right);
    inline psRenderEllipse& operator =(const psCircle& right) { operator=(psEllipse(right.x, right.y, right.r, right.r)); return *this; }
    inline psRenderEllipse& operator =(const psEllipse& right) { SetPosition(right.pos.x - right.a, right.pos.y - right.b); SetDim(psVec(right.a * 2, right.b * 2)); return *this; }

    static inline void DrawCircle(const psCircle& p, uint32_t color) { DrawEllipse(p.x, p.y, p.r, p.r, color); }
    static inline void DrawCircle(float x, float y, float r, uint32_t color) { DrawEllipse(x, y, r, r, color); }
    static inline void DrawEllipse(const psEllipse& p, uint32_t color) { DrawEllipse(p.x, p.y, p.a, p.b, color); }
    static void DrawEllipse(float x, float y, float a, float b, uint32_t color);

  protected:
    virtual void _render(const psTransform2D& parent) override;

    psColor32 _color;
  };

  class PS_DLLEXPORT psRenderLine : public psLocatable, public psRenderable, public psDriverHold
  {
  public:
    psRenderLine(const psRenderLine& copy);
    psRenderLine(psRenderLine&& mov);
    explicit psRenderLine(const psLine3D& line);
    explicit psRenderLine(const psLine& line);
    inline psColor32 GetColor() const { return _color; }
    inline void SetColor(psColor32 color) { _color = color; }

    inline operator psLine3D() const { return psLine3D(position, _point); }
    psRenderLine& operator =(const psRenderLine& right);
    psRenderLine& operator =(psRenderLine&& right);
    psRenderLine& operator =(const psLine3D& right);
    psRenderLine& operator =(const psLine& right);

    static void DrawLine(const psLine3D& p, uint32_t color);
    static inline void DrawLine(const psLine& p, uint32_t color) { DrawLine(psLine3D(p.x1, p.y1, 0, p.x2, p.y2, 0), color); }

  protected:
    virtual void _render(const psTransform2D& parent) override;

    psVec3D _point;
    psColor32 _color;
  };

  class PS_DLLEXPORT psRenderPolygon : public psLocatable, public psRenderable, public psPolygon, public psDriverHold
  {
  public:
    psRenderPolygon(const psRenderPolygon& copy);
    psRenderPolygon(psRenderPolygon&& mov);
    explicit psRenderPolygon(const psPolygon& polygon, uint32_t color = 0xFFFFFFFF);
    explicit psRenderPolygon(psPolygon&& polygon, uint32_t color = 0xFFFFFFFF);
    inline psColor32 GetColor() const { return _color; }
    inline void SetColor(psColor32 color) { _color = color; }

    psRenderPolygon& operator =(const psRenderPolygon& right);
    psRenderPolygon& operator =(psRenderPolygon&& right);
    psRenderPolygon& operator =(const psPolygon& right);

    static void DrawPolygon(const psVec* p, uint32_t num, uint32_t color, const psVec3D& offset = VEC3D_ZERO);
    static void DrawPolygon(const psVertex* p, uint32_t num);

  protected:
    virtual void _render(const psTransform2D& parent) override;

    psColor32 _color;
  };

  class PS_DLLEXPORT psFullScreenQuad : public psRenderable, public psTextured, public psDriverHold
  {
  public:
    psFullScreenQuad(const psFullScreenQuad& copy);
    psFullScreenQuad(psFullScreenQuad&& mov);
    psFullScreenQuad();
    inline psColor32 GetColor() const { return _color; }
    inline void SetColor(psColor32 color) { _color = color; }

  protected:
    virtual void _render(const psTransform2D& parent) override;

    psColor32 _color;
  };
}
#endif