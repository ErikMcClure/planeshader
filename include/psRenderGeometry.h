// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __RENDER_GEOMETRY_H__PS__
#define __RENDER_GEOMETRY_H__PS__

#include "psLine.h"
#include "psSolid.h"
#include "psPolygon.h"
#include "psEllipse.h"
#include "psColored.h"
#include "psDriver.h"
#include "psTextured.h"

namespace planeshader {
  class PS_DLLEXPORT psRenderEllipse : public psSolid, public psColored, public psDriverHold
  {
  public:
    psRenderEllipse(const psRenderEllipse& copy);
    psRenderEllipse(psRenderEllipse&& mov);
    explicit psRenderEllipse(const psCircle& circle);
    explicit psRenderEllipse(const psEllipse& ellipse);

    inline operator psEllipse() const { return psEllipse(_relpos.x + _realdim.x, _relpos.y + _realdim.y, _realdim.x/2, _realdim.y/2); }
    psRenderEllipse& operator =(const psRenderEllipse& right);
    psRenderEllipse& operator =(psRenderEllipse&& right);
    inline psRenderEllipse& operator =(const psCircle& right) { operator=(psEllipse(right.x, right.y, right.r, right.r)); return *this; }
    inline psRenderEllipse& operator =(const psEllipse& right) { SetPosition(right.pos.x - right.a, right.pos.y - right.b); SetDim(psVec(right.a * 2, right.b * 2)); return *this; }

    static inline void DrawCircle(const psCircle& p, unsigned int color) { DrawEllipse(p.x, p.y, p.r, p.r, color); }
    static inline void DrawCircle(float x, float y, float r, unsigned int color) { DrawEllipse(x, y, r, r, color); }
    static inline void DrawEllipse(const psEllipse& p, unsigned int color) { DrawEllipse(p.x, p.y, p.a, p.b, color); }
    static void DrawEllipse(float x, float y, float a, float b, unsigned int color);

  protected:
    virtual void _render();
    virtual void _renderbatch();
    virtual void BSS_FASTCALL _renderbatchlist(psRenderable** rlist, unsigned int count);
    virtual bool BSS_FASTCALL _batch(psRenderable* r) const;
  };

  class PS_DLLEXPORT psRenderLine : public psInheritable, public psColored, public psDriverHold
  {
  public:
    psRenderLine(const psRenderLine& copy);
    psRenderLine(psRenderLine&& mov);
    explicit psRenderLine(const psLine3D& line);
    explicit psRenderLine(const psLine& line);

    inline operator psLine3D() const { return psLine3D(_relpos, _point); }
    psRenderLine& operator =(const psRenderLine& right);
    psRenderLine& operator =(psRenderLine&& right);
    psRenderLine& operator =(const psLine3D& right);
    psRenderLine& operator =(const psLine& right);

    static void DrawLine(const psLine3D& p, unsigned int color);
    static inline void DrawLine(const psLine& p, unsigned int color) { DrawLine(psLine3D(p.x1, p.y1, 0, p.x2, p.y2, 0), color); }

  protected:
    virtual void _render();
    virtual void _renderbatch();
    virtual void BSS_FASTCALL _renderbatchlist(psRenderable** rlist, unsigned int count);
    virtual bool BSS_FASTCALL _batch(psRenderable* r) const;

    psVec3D _point;
  };

  class PS_DLLEXPORT psRenderPolygon : public psInheritable, public psPolygon, public psColored, public psDriverHold
  {
  public:
    psRenderPolygon(const psRenderPolygon& copy);
    psRenderPolygon(psRenderPolygon&& mov);
    explicit psRenderPolygon(const psPolygon& polygon, unsigned int color = 0xFFFFFFFF);
    explicit psRenderPolygon(psPolygon&& polygon, unsigned int color = 0xFFFFFFFF);

    psRenderPolygon& operator =(const psRenderPolygon& right);
    psRenderPolygon& operator =(psRenderPolygon&& right);
    psRenderPolygon& operator =(const psPolygon& right);

    static void DrawPolygon(const psVec* p, size_t num, unsigned int color, const psVec3D& offset = VEC3D_ZERO);
    static void DrawPolygon(const psVertex* p, size_t num, const float(&transform)[4][4] = psDriver::identity);

  protected:
    virtual void _render();
  };

  class PS_DLLEXPORT psFullScreenQuad : public psInheritable, public psTextured, public psColored, public psDriverHold
  {
    psFullScreenQuad(const psFullScreenQuad& copy);
    psFullScreenQuad(psFullScreenQuad&& mov);
    psFullScreenQuad();
    virtual psTex* const* GetTextures() const { return psTextured::GetTextures(); }
    virtual unsigned char NumTextures() const { return psTextured::NumTextures(); }

  protected:
    virtual void _render();
  };
}
#endif