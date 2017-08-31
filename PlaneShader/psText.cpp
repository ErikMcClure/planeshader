// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#include "psText.h"

using namespace planeshader;
using namespace bss;

psText::psText(const psText& copy) : psSolid(copy), _color(copy._color), _text(copy._text), _font(copy._font), _textdim(copy._textdim),
  _letterspacing(copy._letterspacing), _drawflags(copy._drawflags), _func(copy._func), _lineheight(copy._lineheight)
{ 
}
psText::psText(psText&& mov) : psSolid(std::move(mov)), _color(mov._color), _text(std::move(mov._text)), _font(std::move(mov._font)), _textdim(mov._textdim),
_letterspacing(mov._letterspacing), _drawflags(mov._drawflags), _func(mov._func), _lineheight(mov._lineheight)
{
}
psText::psText(psTexFont* font, const char* text, float lineheight, const psVec3D& position, FNUM rotation, const psVec& pivot, psFlag flags, int zorder, psStateblock* stateblock, psShader* shader, psPass* pass, const psVec& scale) :
  psSolid(position, rotation, pivot, flags, zorder, stateblock, shader, pass, scale), _color(0xFFFFFFFF), _text(text), _font(font), _textdim(-1, -1),
  _letterspacing(0), _drawflags(0), _func(0, 0), _lineheight((lineheight) == 0.0f ? (font ? font->GetLineHeight() : 0.0f) : lineheight)
{
}
psText::~psText() { }
psText& psText::operator=(const psText& copy)
{
  psSolid::operator=(copy);
  _color = copy._color;
  _text = copy._text;
  _font = copy._font;
  _textdim = copy._textdim;
  _letterspacing = copy._letterspacing;
  _drawflags = copy._drawflags;
  _func = copy._func;
  _lineheight = copy._lineheight;
  return *this;
}
psText& psText::operator=(psText&& mov)
{
  psSolid::operator=(std::move(mov));
  _color = mov._color;
  _text = std::move(mov._text);
  _font = std::move(mov._font);
  _textdim = mov._textdim;
  _letterspacing = mov._letterspacing;
  _drawflags = mov._drawflags;
  _func = mov._func;
  _lineheight = mov._lineheight;
  return *this;
}

void psText::SetFont(psTexFont* font)
{ 
  _font = font;
}
void psText::_render(const psTransform2D& parent)
{
  if(_font)
  {
    psVec3D pos = parent.CalcPosition(*this);
    Matrix<float, 4, 4> m;
    Matrix<float, 4, 4>::AffineTransform_T(pos.x - GetPivot().x, pos.y - GetPivot().y, pos.z, parent.rotation + rotation, GetPivot().x, GetPivot().y, m);
    // This mimics assembling a scaling matrix and multiplying it with m, assuming we are using transposed matrices.
    sseVec(m.v[0])*sseVec(_scale.x) >> m.v[0];
    sseVec(m.v[1])*sseVec(_scale.y) >> m.v[1];
    psDriverHold::GetDriver()->PushTransform(m.v);
    _textdim = _font->DrawText(GetShader(), _stateblock, _text.c_str(), _lineheight, _letterspacing, psRectRotateZ(0, 0, GetUnscaledDim().x, GetUnscaledDim().y, 0), GetColor().color, GetFlags(), _func);
    psDriverHold::GetDriver()->PopTransform();
  }
}

void psText::_recalcdim()
{
  psVec d(_textdim);
  if(_font)
    _font->CalcTextDim(_text.c_str(), d, _lineheight, _drawflags, _letterspacing);
  if(_textdim.x >= 0)
    d.x = _textdim.x;
  if(_textdim.y >= 0)
    d.y = _textdim.y;
  SetDim(d);
}