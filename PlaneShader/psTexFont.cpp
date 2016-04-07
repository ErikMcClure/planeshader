// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psTexFont.h"
#include "psTex.h"
#include "bss-util/cStr.h"

using namespace planeshader;

psVec psTexFont::DrawText(psShader* shader, const psStateblock* stateblock, const int* text, const psRect& prearea, unsigned short drawflags, FNUM Z, unsigned int color, psFlag flags, psVec dim, float letterspacing, DELEGATE d, const float(&transform)[4][4])
{
  float linewidth;
  float curwidth = 0.0f;
  float linestart;
  psVec cur;
  char c;
  const psGlyph* g;
  const int* pos;
  const int* peek = text;
  psRect area(prearea);
  psVec maxdim = area.GetDimensions();
  psVec texdim;
  unsigned char svar=_textures.Capacity();
  psRectRotateZ rect(0, 0, 0, 0, 0, VEC_ZERO, Z);

  if((dim.y == 0.0f && (drawflags&TDT_BOTTOM || drawflags&TDT_VCENTER || drawflags&TDT_CLIP)) ||
    (dim.x == 0.0f && (drawflags&TDT_RIGHT || drawflags&TDT_CENTER || drawflags&TDT_CLIP)))
  {
    dim = VEC_ZERO;
    while(*peek)
    {
      linewidth = _getlinewidth(peek, maxdim.x, drawflags, letterspacing, curwidth);
      if(linewidth > dim.x) dim.x = linewidth;
      dim.y += _lineheight;
    }
  }
  if(maxdim.x == 0.0f) maxdim.x = dim.x;
  if(maxdim.y == 0.0f) maxdim.y = dim.y;
  area.right = area.left + maxdim.x;
  area.bottom = area.top + maxdim.y;

  if(drawflags&TDT_CLIP) //this applies a clip rect to clip the text, but only within the currently defined clip rect.
  {
    psRect transfer;
    if(!(flags&PSFLAG_FIXED)) {
      transfer.topleft = _driver->TransformPoint(psVec3D(area.top, area.left, 0)).xy;
      transfer.bottomright = _driver->TransformPoint(psVec3D(area.bottom, area.right, 0)).xy;
    }
    _driver->MergeClipRect(transfer);
  }

  // TODO: Skip forward based on the position of the scissor rect as queried by PeekClipRect, regardless of whether TDT_CLIP is specified or not.
  const int* begin;

  for(unsigned char i = 0; i < svar; ++i)
  {
    pos = text;
    peek = text;

    if(drawflags&TDT_BOTTOM)
      cur.y = area.bottom - dim.y;
    else if(drawflags&TDT_VCENTER)
      cur.y = area.top + (maxdim.y - dim.y)*0.5f;
    else
      cur.y = area.top;

    curwidth = 0.0f;
    _driver->SetTextures(&_textures[i], 1);
    psBatchObj& obj = _driver->DrawRectBatchBegin(shader, stateblock, 1, flags, transform);
    texdim = _textures[i]->GetDim();

    while(*pos)
    {
      begin = pos;
      linewidth = _getlinewidth(peek, maxdim.x, drawflags, letterspacing, curwidth) + area.left;

      if(drawflags&TDT_RIGHT)
        cur.x = area.right - linewidth;
      else if(drawflags&TDT_CENTER)
        cur.x = area.left + (maxdim.x - linewidth)*0.5f;
      else
        cur.x = area.left;
      linestart = cur.x;

      while(*pos)
      {
        size_t ipos = pos - text;
        c = *pos;
        g = _glyphs[c];
        if(!g) { ++pos; continue; } // note: we don't bother attempting to load glyphs here because they should've already been loaded by _getlinewidth
        if(begin != pos && (cur.x + g->width + letterspacing) > linewidth) break; // Force all lines to have at least one character
        ++pos;
        if(g->texnum != i) { cur.x += g->width + letterspacing; continue; }

        rect.left = cur.x + g->advance;
        rect.top= cur.y + g->ascender + _lineheight;

        if(flags&TDT_RTL) // implement right to left rendering by flipping over the middle axis
        {
          rect.left = linewidth - (rect.left - linestart) - (g->uv.right-g->uv.left)*texdim.x;
        }
        if(flags&TDT_PIXELSNAP)
        {
          rect.left = (float)bss_util::fFastRound(rect.left);
          rect.top = (float)bss_util::fFastRound(rect.top);
        }

        rect.right = rect.left + (g->uv.right-g->uv.left)*texdim.x;
        rect.bottom = rect.top + (g->uv.bottom-g->uv.top)*texdim.y;

        if(!d.IsEmpty()) d(ipos, rect, color);
        _driver->DrawRectBatch(obj, rect, &g->uv, color);

        cur.x += g->width + letterspacing;
      }
      cur.y += _lineheight;
    }
  }

  if(drawflags&TDT_CLIP)
    _driver->PopClipRect();

  return dim;
}

psVec psTexFont::DrawText(psShader* shader, const psStateblock* stateblock, const char* text, const psRect& area, unsigned short drawflags, FNUM Z, unsigned int color, psFlag flags, psVec dim, float letterspacing, DELEGATE d, const float(&transform)[4][4])
{
  size_t len = strlen(text)+1;
  if(len < 1000000)
  {
    DYNARRAY(int, txt, len);
    UTF8toUTF32(text, txt, len);
    return DrawText(shader, stateblock, txt, area, drawflags, Z, color, flags, dim, letterspacing, d, transform);
  }
  cStrT<int> txt(text);
  return DrawText(shader, stateblock, txt, area, drawflags, Z, color, flags, dim, letterspacing, d, transform);
}
bool psTexFont::_isspace(int c) // We have to make our own isspace implementation because the standard isspace() explodes if you feed it unicode characters.
{
  return c == ' ' || c == '\t' ||c == '\n' ||c == '\r' ||c == '\v' ||c == '\f';
}
float psTexFont::_getlinewidth(const int*& text, float maxwidth, unsigned short drawflags, float letterspacing, float& cur)
{
  bool dobreak = maxwidth > 0 && (drawflags&TDT_CHARBREAK || drawflags&TDT_WORDBREAK);
  float width = cur;
  int c;
  const psGlyph* g;
  while(*text)
  {
    c = *text;
    g = _glyphs[c];
    if(!g && c != '\n' && c != '\r')
    {
      g = _loadglyph(c);
      if(!g) continue; // Note: Bad glyphs usually just have 0 width, so we don't have to check for them.
    }
    if(c == '\n' || (dobreak && (cur + g->width + letterspacing) > maxwidth)) // We use >= here instead of > due to nasty precision issues at high DPI levels
    {
      cur -= width;
      if(c != '\n')
        cur += g->width + letterspacing;
      ++text;
      return width + 0.01f;
    }
    if(!(drawflags&TDT_WORDBREAK) || _isspace(c))
      width = cur + g->width + letterspacing;
    cur += g->width + letterspacing;

    ++text;
  }
  return cur;
}
psGlyph* psTexFont::_loadglyph(unsigned int codepoint)
{
  return 0;
}

void psTexFont::CalcTextDim(const int* text, psVec& dest, unsigned short drawflags, float letterspacing)
{
  float cur = 0.0f;
  float height = (!text || !text[0])?_lineheight:0.0f;
  psVec maxdim = dest;
  float width = 0.0f;

  float w;
  while(*text)
  {
    w = _getlinewidth(text, maxdim.x, drawflags, letterspacing, cur);
    if(w > width) width = w;
    height += _lineheight;
  }

  if(maxdim.x < 0.0f) dest.x = width;
  if(maxdim.y < 0.0f) dest.y = height;
}

void psTexFont::AddGlyph(int character, const psGlyph& glyph)
{
  _glyphs.Insert(character, glyph);
}

unsigned char psTexFont::AddTexture(psTex* tex){ tex->Grab(); _textures.Insert(tex, _textures.Capacity()); return _textures.Capacity()-1; }
void psTexFont::DestroyThis(){ delete this; }

psTexFont* psTexFont::CreateTexFont(psTex* tex, float lineheight)
{
  return new psTexFont(tex, lineheight);
}

psTexFont::psTexFont(const psTexFont& copy) : _lineheight(copy._lineheight), _glyphs(copy._glyphs)
{
  for(int i = 0; i < copy._textures.Capacity(); ++i)
  {
    copy._textures[i]->Grab();
    _textures.Insert(copy._textures[i], _textures.Capacity());
  }
}
psTexFont::psTexFont(psTexFont&& mov) : _lineheight(mov._lineheight), _textures(std::move(mov._textures)), _glyphs(std::move(mov._glyphs)) {}
psTexFont::psTexFont(psTex* tex, float lineheight) : _lineheight(lineheight), _textures(1) { _textures[0] = tex; if(_textures[0]) _textures[0]->Grab(); Grab(); }
psTexFont::psTexFont(float lineheight) : _lineheight(lineheight) { Grab(); }
psTexFont::~psTexFont() { for(unsigned int i = 0; i < _textures.Capacity(); ++i) _textures[i]->Drop(); }