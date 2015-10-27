// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psRenderGeometry.h"
#include "psShader.h"

using namespace planeshader;

psRenderEllipse::psRenderEllipse(const psRenderEllipse& copy) : psSolid(copy), psColored(copy) {}
psRenderEllipse::psRenderEllipse(psRenderEllipse&& mov) : psSolid(std::move(mov)), psColored(std::move(mov)) {}
psRenderEllipse::psRenderEllipse(const psCircle& circle) : psSolid(VEC3D_ZERO, 0, VEC_ZERO, 0, 0, 0, _driver->library.CIRCLE, 0, 0, VEC_ONE, psRenderable::INTERNALTYPE_ELLIPSE) { operator=(circle); }
psRenderEllipse::psRenderEllipse(const psEllipse& ellipse) : psSolid(VEC3D_ZERO, 0, VEC_ZERO, 0, 0, 0, _driver->library.CIRCLE, 0, 0, VEC_ONE, psRenderable::INTERNALTYPE_ELLIPSE) { operator=(ellipse); }

psRenderEllipse& psRenderEllipse::operator =(const psRenderEllipse& right) { psSolid::operator=(right); psColored::operator=(right); return *this; }
psRenderEllipse& psRenderEllipse::operator =(psRenderEllipse&& right) { psSolid::operator=(std::move(right)); psColored::operator=(std::move(right)); return *this; }

void psRenderEllipse::DrawEllipse(float x, float y, float a, float b, unsigned int color)
{
  _driver->SetStateblock(0);
  _driver->library.CIRCLE->Activate();
  _driver->DrawRect(psRectRotateZ(x-a, y-b, x+a, y+b, 0), 0, 0, color, 0, 0, 0);
}

void psRenderEllipse::_renderbatch()
{
  _driver->DrawRectBatch(GetCollisionRect(), 0, GetColor());
}
void BSS_FASTCALL psRenderEllipse::_renderbatchlist(psRenderable** rlist, unsigned int count)
{
  _driver->DrawRectBatchBegin(0, 0, 0, GetAllFlags());

  for(size_t i = 0; i < count; ++i)
    rlist[i]->_renderbatch();

  _driver->DrawRectBatchEnd();
}
bool BSS_FASTCALL psRenderEllipse::_batch(psRenderable* r) const { return (GetAllFlags()&PSFLAG_BATCHFLAGS)==(r->GetAllFlags()&PSFLAG_BATCHFLAGS); }

void psRenderEllipse::_render()
{
  _driver->DrawRect(GetCollisionRect(), 0, 0, GetColor(), 0, 0, GetAllFlags());
}

psRenderLine::psRenderLine(const psRenderLine& copy) : psInheritable(copy), psColored(copy) {}
psRenderLine::psRenderLine(psRenderLine&& mov) : psInheritable(std::move(mov)), psColored(std::move(mov)) {}
psRenderLine::psRenderLine(const psLine3D& line) : psInheritable(VEC3D_ZERO, 0, VEC_ZERO, 0, 0, 0, _driver->library.LINE, 0, 0, psRenderable::INTERNALTYPE_LINE) { operator=(line); }
psRenderLine::psRenderLine(const psLine& line) : psInheritable(VEC3D_ZERO, 0, VEC_ZERO, 0, 0, 0, _driver->library.LINE, 0, 0, psRenderable::INTERNALTYPE_LINE) { operator=(line); }

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
void psRenderLine::DrawLine(const psLine3D& p, unsigned int color)
{
  _driver->SetStateblock(0);
  _driver->library.LINE->Activate();
  _driver->DrawLinesStart(0);
  _driver->DrawLines(psLine(p.p1.xy, p.p2.xy), p.p1.z, p.p2.z, color);
  _driver->DrawLinesEnd();
}

void psRenderLine::_render()
{
  _driver->DrawLinesStart(GetAllFlags());
  _renderbatch();
  _driver->DrawLinesEnd();
}
void psRenderLine::_renderbatch()
{
  _driver->DrawLines(psLine(_relpos.xy, _point.xy), _relpos.z, _point.z, GetColor().color);
}
void BSS_FASTCALL psRenderLine::_renderbatchlist(psRenderable** rlist, unsigned int count)
{
  _driver->DrawLinesStart(GetAllFlags());

  for(size_t i = 0; i < count; ++i)
    rlist[i]->_renderbatch();

  _driver->DrawLinesEnd();
}
bool BSS_FASTCALL psRenderLine::_batch(psRenderable* r) const { return (GetAllFlags()&PSFLAG_BATCHFLAGS) == (r->GetAllFlags()&PSFLAG_BATCHFLAGS); }

psRenderPolygon::psRenderPolygon(const psRenderPolygon& copy) : psInheritable(copy), psPolygon(copy), psColored(copy) {}
psRenderPolygon::psRenderPolygon(psRenderPolygon&& mov) : psInheritable(std::move(mov)), psPolygon(std::move(mov)), psColored(std::move(mov)) {}
psRenderPolygon::psRenderPolygon(const psPolygon& polygon, unsigned int color) : psInheritable(VEC3D_ZERO, 0, VEC_ZERO, 0, 0, 0, _driver->library.POLYGON, 0, 0, psRenderable::INTERNALTYPE_POLYGON), psPolygon(polygon), psColored(color) {}
psRenderPolygon::psRenderPolygon(psPolygon&& polygon, unsigned int color) : psInheritable(VEC3D_ZERO, 0, VEC_ZERO, 0, 0, 0, _driver->library.POLYGON, 0, 0, psRenderable::INTERNALTYPE_POLYGON), psPolygon(std::move(polygon)), psColored(color) {}

psRenderPolygon& psRenderPolygon::operator =(const psRenderPolygon& right) { psInheritable::operator=(right); psPolygon::operator=(right); psColored::operator=(right); return *this; }
psRenderPolygon& psRenderPolygon::operator =(psRenderPolygon&& right) { psInheritable::operator=(std::move(right)); psPolygon::operator=(std::move(right)); psColored::operator=(std::move(right)); return *this; }
psRenderPolygon& psRenderPolygon::operator =(const psPolygon& polygon) { psPolygon::operator=(polygon); return *this; }

void psRenderPolygon::DrawPolygon(const psVec* p, size_t num, unsigned int color, const psVec3D& offset) { _driver->SetStateblock(0); _driver->library.POLYGON->Activate(); _driver->DrawPolygon(p, num, offset, color, 0); }
void psRenderPolygon::DrawPolygon(const psVertex* p, size_t num, const float(&transform)[4][4]) { _driver->SetStateblock(0); _driver->library.POLYGON->Activate(); _driver->DrawPolygon(p, num, 0, transform); }

void psRenderPolygon::_render()
{
  psVec3D pos;
  GetTotalPosition(pos);
  bss_util::Matrix<float, 4, 4> m;
  bss_util::Matrix<float, 4, 4>::AffineTransform_T(pos.x, pos.y, pos.z, GetTotalRotation(), GetPivot().x, GetPivot().y, m);
  _driver->DrawPolygon(_verts, _verts.Capacity(), VEC3D_ZERO, GetColor().color, GetAllFlags());
}