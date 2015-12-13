// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psText.h"

using namespace planeshader;

psText::psText(const psText& copy) : psSolid(copy), psColored(copy), _text(copy._text), _font(copy._font), _textdim(copy._textdim),
  _letterspacing(copy._letterspacing), _drawflags(copy._drawflags), _func(copy._func)
{ 
}
psText::psText(psText&& mov) : psSolid(std::move(mov)), psColored(std::move(mov)), _text(std::move(mov._text)), _font(std::move(mov._font)), _textdim(mov._textdim),
_letterspacing(mov._letterspacing), _drawflags(mov._drawflags), _func(mov._func)
{
}
psText::psText(psTexFont* font, const char* text, const psVec3D& position, FNUM rotation, const psVec& pivot, FLAG_TYPE flags, int zorder, psStateblock* stateblock, psShader* shader, psPass* pass, psInheritable* parent, const psVec& scale) :
  psSolid(position, rotation, pivot, flags, zorder, stateblock, shader, pass, parent, scale, psRenderable::INTERNALTYPE_TEXT), psColored(0xFFFFFFFF), _text(text), _font(font), _textdim(-1, -1),
  _letterspacing(0), _drawflags(0), _func(0, 0)
{
}
psText::~psText() { }
void psText::SetFont(psTexFont* font)
{ 
  _font = font;
}
void psText::_render()
{
  if(_font)
  {
    psVec3D pos;
    GetTotalPosition(pos);
    bss_util::Matrix<float, 4, 4> m;
    bss_util::Matrix<float, 4, 4>::AffineTransform_T(pos.x - GetPivot().x, pos.y - GetPivot().y, pos.z, GetTotalRotation(), GetPivot().x, GetPivot().y, m);
    // This mimics assembling a scaling matrix and multiplying it with m, assuming we are using transposed matrices.
    sseVec(m.v[0])*sseVec(_scale.x) >> m.v[0];
    sseVec(m.v[1])*sseVec(_scale.y) >> m.v[1];
    _font->DrawText(_text.c_str(), psRect(VEC_ZERO, GetUnscaledDim()), _drawflags, 0, GetColor().color, GetAllFlags(), _textdim, _letterspacing, _func, m.v);
  }
}

void psText::_recalcdim()
{
  psVec d(_textdim);
  if(_font)
    _font->CalcTextDim(_text.c_str(), d, _drawflags, _letterspacing);
  SetDim(d);
}