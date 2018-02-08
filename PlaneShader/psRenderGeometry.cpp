// Copyright ©2018 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#include "psEngine.h"
#include "psRenderGeometry.h"
#include "psShader.h"
#include "psCamera.h"

using namespace planeshader;

psRenderEllipse::psRenderEllipse(const psRenderEllipse& copy) : psSolid(copy), _color(copy._color) {}
psRenderEllipse::psRenderEllipse(psRenderEllipse&& mov) : psSolid(std::move(mov)), _color(mov._color) {}
psRenderEllipse::psRenderEllipse(const psCircle& circle) : psSolid(VEC3D_ZERO, 0, VEC_ZERO, 0, 0, 0, _driver->library.CIRCLE, 0, VEC_ONE), _color(0xFFFFFFFF) { operator=(circle); }
psRenderEllipse::psRenderEllipse(const psEllipse& ellipse) : psSolid(VEC3D_ZERO, 0, VEC_ZERO, 0, 0, 0, _driver->library.CIRCLE, 0, VEC_ONE), _color(0xFFFFFFFF) { operator=(ellipse); }

psRenderEllipse& psRenderEllipse::operator =(const psRenderEllipse& right) { psSolid::operator=(right); _color = right._color; return *this; }
psRenderEllipse& psRenderEllipse::operator =(psRenderEllipse&& right) { psSolid::operator=(std::move(right)); _color = right._color; return *this; }

void psRenderEllipse::DrawEllipse(float x, float y, float a, float b, uint32_t color)
{
  _driver->SetTextures(0, 0);
  _driver->DrawRect(_driver->library.CIRCLE, 0, psRectRotateZ(x-a, y-b, x+a, y+b, 0), 0, 0, color, 0);
}

void psRenderEllipse::_render(const psTransform2D& parent)
{
  _driver->DrawRect(_driver->library.CIRCLE, GetStateblock(), GetCollisionRect(parent), 0, 0, GetColor(), GetFlags());
}

psRenderLine::psRenderLine(const psRenderLine& copy) : psLocatable(copy), psRenderable(copy), _color(copy._color) {}
psRenderLine::psRenderLine(psRenderLine&& mov) : psLocatable(std::move(mov)), psRenderable(std::move(mov)), _color(mov._color) {}
psRenderLine::psRenderLine(const psLine3D& line) : psLocatable(VEC3D_ZERO, 0, VEC_ZERO), psRenderable(0, 0, 0, _driver->library.LINE, 0), _color(0xFFFFFFFF) { operator=(line); }
psRenderLine::psRenderLine(const psLine& line) : psLocatable(VEC3D_ZERO, 0, VEC_ZERO), psRenderable(0, 0, 0, _driver->library.LINE, 0), _color(0xFFFFFFFF) { operator=(line); }

psRenderLine& psRenderLine::operator =(const psRenderLine& right)
{ 
  psLocatable::operator=(right);
  psRenderable::operator=(right);
  _color = right._color;
  _point = right._point;
  return *this; 
}
psRenderLine& psRenderLine::operator =(psRenderLine&& right)
{ 
  psLocatable::operator=(std::move(right));
  psRenderable::operator=(std::move(right));
  _color = right._color;  
  _point = right._point; 
  return *this; 
}
psRenderLine& psRenderLine::operator =(const psLine3D& right) { SetPosition(right.p1); _point = right.p2; return *this; }
psRenderLine& psRenderLine::operator =(const psLine& right) { operator=(psLine3D(right.x1, right.y1, 0, right.x2, right.y2, 0)); return *this; }
void psRenderLine::DrawLine(const psLine3D& p, uint32_t color)
{
  psBatchObj* obj = _driver->DrawLinesStart(_driver->library.LINE, 0, 0);
  _driver->DrawLines(obj, psLine(p.p1.xy, p.p2.xy), p.p1.z, p.p2.z, color);
}

void psRenderLine::_render(const psTransform2D& parent)
{
  psBatchObj* obj = _driver->DrawLinesStart(_driver->library.LINE, GetStateblock(), GetFlags());
  psTransform2D p = parent.Push(*this);
  _driver->DrawLines(obj, psLine(p.position.xy, _point.xy), p.position.z, _point.z, GetColor().color);
}

psRenderPolygon::psRenderPolygon(const psRenderPolygon& copy) : psLocatable(copy), psRenderable(copy), psPolygon(copy), _color(copy._color) {}
psRenderPolygon::psRenderPolygon(psRenderPolygon&& mov) : psLocatable(std::move(mov)), psRenderable(std::move(mov)), psPolygon(std::move(mov)), _color(mov._color) {}
psRenderPolygon::psRenderPolygon(const psPolygon& polygon, uint32_t color) : psLocatable(VEC3D_ZERO, 0, VEC_ZERO), psRenderable(0, 0, 0, _driver->library.POLYGON, 0), psPolygon(polygon), _color(color) {}
psRenderPolygon::psRenderPolygon(psPolygon&& polygon, uint32_t color) : psLocatable(VEC3D_ZERO, 0, VEC_ZERO), psRenderable(0, 0, 0, _driver->library.POLYGON, 0), psPolygon(std::move(polygon)), _color(color) {}

psRenderPolygon& psRenderPolygon::operator =(const psRenderPolygon& right) 
{
  psLocatable::operator=(right);
  psRenderable::operator=(right);
  psPolygon::operator=(right);
  _color = right._color;
  return *this;
}
psRenderPolygon& psRenderPolygon::operator =(psRenderPolygon&& right)
{
  psLocatable::operator=(std::move(right));
  psRenderable::operator=(std::move(right));
  psPolygon::operator=(std::move(right));
  _color = right._color;
  return *this;
}
psRenderPolygon& psRenderPolygon::operator =(const psPolygon& polygon) { psPolygon::operator=(polygon); return *this; }

void psRenderPolygon::DrawPolygon(const psVec* p, uint32_t num, uint32_t color, const psVec3D& offset) {  _driver->DrawPolygon(_driver->library.POLYGON, 0, p, num, offset, color, 0); }
void psRenderPolygon::DrawPolygon(const psVertex* p, uint32_t num) { _driver->DrawPolygon(_driver->library.POLYGON, 0, p, num, 0); }

void psRenderPolygon::_render(const psTransform2D& parent)
{
  psMatrix m;
  parent.Push(*this).GetMatrix(m);
  _driver->PushTransform(m);
  _driver->DrawPolygon(GetShader(), GetStateblock(), _verts, _verts.Capacity(), VEC3D_ZERO, GetColor().color, GetFlags());
  _driver->PopTransform();
}

psFullScreenQuad::psFullScreenQuad(const psFullScreenQuad& copy) : psRenderable(copy), _color(copy._color) {}
psFullScreenQuad::psFullScreenQuad(psFullScreenQuad&& mov) : psRenderable(std::move(mov)), _color(mov._color) {}
psFullScreenQuad::psFullScreenQuad() : psRenderable(PSFLAG_FIXED), _color(0xFFFFFFFF) {}
void psFullScreenQuad::_render(const psTransform2D& parent)
{
  _driver->SetTextures(GetTextures(), NumTextures(), PIXEL_SHADER_1_1);
  _driver->DrawFullScreenQuad();
}