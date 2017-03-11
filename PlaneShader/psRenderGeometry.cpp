// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psRenderGeometry.h"
#include "psShader.h"
#include "psCamera.h"

using namespace planeshader;

psRenderEllipse::psRenderEllipse(const psRenderEllipse& copy) : psSolid(copy), psColored(copy) {}
psRenderEllipse::psRenderEllipse(psRenderEllipse&& mov) : psSolid(std::move(mov)), psColored(std::move(mov)) {}
psRenderEllipse::psRenderEllipse(const psCircle& circle) : psSolid(VEC3D_ZERO, 0, VEC_ZERO, 0, 0, 0, _driver->library.CIRCLE, 0, 0, VEC_ONE) { operator=(circle); }
psRenderEllipse::psRenderEllipse(const psEllipse& ellipse) : psSolid(VEC3D_ZERO, 0, VEC_ZERO, 0, 0, 0, _driver->library.CIRCLE, 0, 0, VEC_ONE) { operator=(ellipse); }

psRenderEllipse& psRenderEllipse::operator =(const psRenderEllipse& right) { psSolid::operator=(right); psColored::operator=(right); return *this; }
psRenderEllipse& psRenderEllipse::operator =(psRenderEllipse&& right) { psSolid::operator=(std::move(right)); psColored::operator=(std::move(right)); return *this; }

void psRenderEllipse::DrawEllipse(float x, float y, float a, float b, uint32_t color)
{
  _driver->SetTextures(0, 0);
  _driver->DrawRect(_driver->library.CIRCLE, 0, psRectRotateZ(x-a, y-b, x+a, y+b, 0), 0, 0, color, 0);
}

void psRenderEllipse::_render()
{
  Activate();
  _driver->DrawRect(_driver->library.CIRCLE, GetStateblock(), GetCollisionRect(), 0, 0, GetColor(), GetAllFlags());
}

psRenderLine::psRenderLine(const psRenderLine& copy) : psInheritable(copy), psColored(copy) {}
psRenderLine::psRenderLine(psRenderLine&& mov) : psInheritable(std::move(mov)), psColored(std::move(mov)) {}
psRenderLine::psRenderLine(const psLine3D& line) : psInheritable(VEC3D_ZERO, 0, VEC_ZERO, 0, 0, 0, _driver->library.LINE, 0, 0) { operator=(line); }
psRenderLine::psRenderLine(const psLine& line) : psInheritable(VEC3D_ZERO, 0, VEC_ZERO, 0, 0, 0, _driver->library.LINE, 0, 0) { operator=(line); }

psRenderLine& psRenderLine::operator =(const psRenderLine& right)
{ 
  psInheritable::operator=(right);
  psColored::operator=(right);
  _point = right._point;
  return *this; 
}
psRenderLine& psRenderLine::operator =(psRenderLine&& right)
{ 
  psInheritable::operator=(std::move(right)); 
  psColored::operator=(std::move(right));  
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

void psRenderLine::_render()
{
  Activate();
  psBatchObj* obj = _driver->DrawLinesStart(_driver->library.LINE, GetStateblock(), GetAllFlags());
  _driver->DrawLines(obj, psLine(_relpos.xy, _point.xy), _relpos.z, _point.z, GetColor().color);
}

psRenderPolygon::psRenderPolygon(const psRenderPolygon& copy) : psInheritable(copy), psPolygon(copy), psColored(copy) {}
psRenderPolygon::psRenderPolygon(psRenderPolygon&& mov) : psInheritable(std::move(mov)), psPolygon(std::move(mov)), psColored(std::move(mov)) {}
psRenderPolygon::psRenderPolygon(const psPolygon& polygon, uint32_t color) : psInheritable(VEC3D_ZERO, 0, VEC_ZERO, 0, 0, 0, _driver->library.POLYGON, 0, 0), psPolygon(polygon), psColored(color) {}
psRenderPolygon::psRenderPolygon(psPolygon&& polygon, uint32_t color) : psInheritable(VEC3D_ZERO, 0, VEC_ZERO, 0, 0, 0, _driver->library.POLYGON, 0, 0), psPolygon(std::move(polygon)), psColored(color) {}

psRenderPolygon& psRenderPolygon::operator =(const psRenderPolygon& right) { psInheritable::operator=(right); psPolygon::operator=(right); psColored::operator=(right); return *this; }
psRenderPolygon& psRenderPolygon::operator =(psRenderPolygon&& right) { psInheritable::operator=(std::move(right)); psPolygon::operator=(std::move(right)); psColored::operator=(std::move(right)); return *this; }
psRenderPolygon& psRenderPolygon::operator =(const psPolygon& polygon) { psPolygon::operator=(polygon); return *this; }

void psRenderPolygon::DrawPolygon(const psVec* p, uint32_t num, uint32_t color, const psVec3D& offset) {  _driver->DrawPolygon(_driver->library.POLYGON, 0, p, num, offset, color, 0); }
void psRenderPolygon::DrawPolygon(const psVertex* p, uint32_t num) { _driver->DrawPolygon(_driver->library.POLYGON, 0, p, num, 0); }

void psRenderPolygon::_render()
{
  psVec3D pos;
  GetTotalPosition(pos);
  bss_util::Matrix<float, 4, 4> m;
  bss_util::Matrix<float, 4, 4>::AffineTransform_T(pos.x, pos.y, pos.z, GetTotalRotation(), GetPivot().x, GetPivot().y, m);
  Activate();
  _driver->DrawPolygon(GetShader(), GetStateblock(), _verts, _verts.Capacity(), VEC3D_ZERO, GetColor().color, GetAllFlags());
}

psFullScreenQuad::psFullScreenQuad(const psFullScreenQuad& copy){}
psFullScreenQuad::psFullScreenQuad(psFullScreenQuad&& mov){}
psFullScreenQuad::psFullScreenQuad(){}
void psFullScreenQuad::_render()
{
  psVec dim = GetRenderTargets()[0]->GetDim();
  _driver->PushCamera(psVec3D(0, 0, -1.0f), VEC_ZERO, 0, psRectiu(0, 0, dim.x, dim.y), psCamera::default_extent);
  _driver->SetTextures(GetTextures(), NumTextures(), PIXEL_SHADER_1_1);
  _driver->DrawFullScreenQuad();
  _driver->PopCamera();
}