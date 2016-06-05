// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psTexFont.h"
#include "psTex.h"
#include "bss-util/cStr.h"

using namespace planeshader;

psVec psTexFont::DrawText(psShader* shader, const psStateblock* stateblock, const int* text, float lineheight, float letterspacing, const psRectRotateZ& prearea, uint32_t color, psFlag flags, DELEGATE d, const float(&transform)[4][4])
{
  float linewidth;
  float curwidth = 0.0f;
  float linestart;
  psVec pen;
  int c;
  const psGlyph* g;
  const int* pos;
  const int* peek = text;
  psRect area(prearea);
  psVec maxdim = area.GetDimensions();
  psVec texdim;
  psVec dim(0, 0);
  uint8_t svar=_textures.Capacity();
  psRectRotateZ rect(0, 0, 0, 0, 0, VEC_ZERO, prearea.z);

  if((flags&PSFONT_BOTTOM || flags&PSFONT_VCENTER || flags&PSFONT_CLIP) ||
    (flags&PSFONT_RIGHT || flags&PSFONT_CENTER || flags&PSFONT_CLIP))
  {
    dim.y = -lineheight + _fontascender - _fontdescender;
    while(*peek)
    {
      linewidth = _getlinewidth(peek, maxdim.x, flags, letterspacing, curwidth);
      if(linewidth > dim.x) dim.x = linewidth;
      dim.y += lineheight;
    }
  }
  if(maxdim.x == 0.0f) maxdim.x = dim.x;
  if(maxdim.y == 0.0f) maxdim.y = dim.y;
  area.right = area.left + maxdim.x;
  area.bottom = area.top + maxdim.y;

  if(flags&PSFONT_CLIP) //this applies a clip rect to clip the text, but only within the currently defined clip rect.
  {
    psRect transfer;
    if(!(flags&PSFLAG_FIXED)) {
      transfer.topleft = _driver->TransformPoint(psVec3D(area.top, area.left, 0)).xy;
      transfer.bottomright = _driver->TransformPoint(psVec3D(area.bottom, area.right, 0)).xy;
    }
    _driver->MergeClipRect(transfer);
  }

  // TODO: Skip forward based on the position of the scissor rect as queried by PeekClipRect, regardless of whether PSFONT_CLIP is specified or not.
  const int* begin;

  for(uint8_t i = 0; i < svar; ++i)
  {
    pos = text;
    peek = text;

    if(flags&PSFONT_BOTTOM)
      pen.y = area.bottom - dim.y;
    else if(flags&PSFONT_VCENTER)
      pen.y = area.top + (maxdim.y - dim.y)*0.5f;
    else
      pen.y = area.top;

    pen.y += (lineheight / _fontlineheight) * _fontascender; // move our top y coordinate position to the actual baseline.
    curwidth = 0.0f;
    _driver->SetTextures(&_textures[i], 1);
    psBatchObj* obj = _driver->DrawRectBatchBegin(shader, stateblock, 1, flags, transform);
    texdim = _textures[i]->GetDim();

    while(*pos)
    {
      begin = pos;
      linewidth = _getlinewidth(peek, maxdim.x, flags, letterspacing, curwidth) + area.left;

      if(flags&PSFONT_RIGHT)
        pen.x = area.right - linewidth;
      else if(flags&PSFONT_CENTER)
        pen.x = area.left + (maxdim.x - linewidth)*0.5f;
      else
        pen.x = area.left;
      linestart = pen.x;
      int last;
      c = 0;

      while(*pos)
      {
        size_t ipos = pos - text;
        last = c;
        c = *pos;
        g = _glyphs[c];
        if(!g) { ++pos; continue; } // note: we don't bother attempting to load glyphs here because they should've already been loaded by _getlinewidth

        float gdimx = (g->uv.right - g->uv.left)*texdim.x;
        float gdimy = (g->uv.bottom - g->uv.top)*texdim.y;

        pen.x += _getkerning(last, c); // We add the kerning amount to the pen position for this character pair first, before doing anything else.
        if(begin != pos && (pen.x + g->bearingX + gdimx + letterspacing) > linewidth + 0.01) break; // Force all lines to have at least one character
        ++pos;
        if(g->texnum != i) { pen.x += g->advance + letterspacing; continue; }

        rect.left = pen.x + g->bearingX;
        rect.top= pen.y - g->bearingY;

        if(flags&PSFONT_RTL) // implement right to left rendering by flipping over the middle axis
        {
          rect.left = linewidth - (rect.left - linestart) - gdimx;
        }
        if(flags&PSFONT_PIXELSNAP)
        {
          rect.left = (float)bss_util::fFastRound(rect.left);
          rect.top = (float)bss_util::fFastRound(rect.top);
        }

        rect.right = rect.left + gdimx;
        rect.bottom = rect.top + gdimy;

        if(!d.IsEmpty()) d(ipos, rect, color);
        _driver->DrawRectBatch(obj, rect, &g->uv, color);

        pen.x += g->advance + letterspacing;
      }
      pen.y += lineheight;
    }
  }

  if(flags&PSFONT_CLIP)
    _driver->PopClipRect();

  return dim;
}

psVec psTexFont::DrawText(psShader* shader, const psStateblock* stateblock, const char* text, float lineheight, float letterspacing, const psRectRotateZ& area, uint32_t color, psFlag flags, DELEGATE d, const float(&transform)[4][4])
{
  size_t len = strlen(text)+1;
  if(len < 100000)
  {
    DYNARRAY(int, txt, len);
    UTF8toUTF32(text, len, txt, len);
    return DrawText(shader, stateblock, txt, lineheight, letterspacing, area, color, flags, d, transform);
  }
  cStrT<int> txt(text);
  return DrawText(shader, stateblock, txt, lineheight, letterspacing, area, color, flags, d, transform);
}
bool psTexFont::_isspace(int c) // We have to make our own isspace implementation because the standard isspace() explodes if you feed it unicode characters.
{
  return c == ' ' || c == '\t' ||c == '\n' ||c == '\r' ||c == '\v' ||c == '\f';
}
float psTexFont::_getlinewidth(const int*& text, float maxwidth, psFlag flags, float letterspacing, float& cur)
{
  bool dobreak = maxwidth > 0 && (flags&PSFONT_CHARBREAK || flags&PSFONT_WORDBREAK);
  float width = cur;
  int c = 0;
  int last = 0;
  float lastadvance = 0;
  float gwidth = 0;
  float advance = 0;
  float kerning = 0;
  const psGlyph* g;

  while(*text)
  {
    last = c;
    c = *text;
    g = _glyphs[c];
    if(!g && c != '\n' && c != '\r')
    {
      g = _loadglyph(c);
      if(!g) continue; // Note: Bad glyphs usually just have 0 width, so we don't have to check for them.
    }
    kerning = _getkerning(last, c);
    gwidth = (c != '\n' && c != '\r') ? g->bearingX + kerning + g->width : 0.0f;
    if(c == '\n' || (dobreak && (cur + gwidth) > maxwidth))
    {
      cur = lastadvance;
      if(c != '\n')
        cur += advance;
      ++text;
      return width;
    }
    advance = g->advance + letterspacing + kerning;
    if(!(flags&PSFONT_WORDBREAK) || _isspace(c))
    {
      width = cur + gwidth;
      lastadvance = cur + advance;
    }
    cur += advance;

    ++text;
  }
  return cur - advance + gwidth;
}
psGlyph* psTexFont::_loadglyph(uint32_t codepoint)
{
  return 0;
}
float psTexFont::_loadkerning(uint32_t prev, uint32_t cur)
{
  return 0.0f;
}
float psTexFont::_getkerning(uint32_t prev, uint32_t c)
{
  if(!prev) return 0;
  uint64_t key = uint64_t(c) | (uint64_t(prev) << 32);
  khiter_t iter = _kerning.Iterator(key);
  if(!_kerning.ExistsIter(iter) && c != '\n' && c != '\r')
  {
    _kerning.Insert(key, _loadkerning(prev, c));
    iter = _kerning.Iterator(key);
  }
  assert(_kerning.ExistsIter(iter));
  return *_kerning.GetValue(iter);
}

void psTexFont::CalcTextDim(const int* text, psVec& dest, float lineheight, float letterspacing, psFlag flags)
{
  float cur = 0.0f;
  psVec maxdim = dest;
  float width = 0.0f;
  int lines = -1; // Line height calculations are only valid if there are two or more lines, so we start the line count at -1

  float w;
  while(*text)
  {
    w = _getlinewidth(text, maxdim.x, flags, letterspacing, cur);
    if(w > width) width = w;
    ++lines;
  }

  if(maxdim.x < 0.0f)
    dest.x = width;
  if(maxdim.y < 0.0f)
    dest.y = _fontascender - _fontdescender + ((lines > 0) ? (lineheight*lines) : 0.0f);
}

void psTexFont::AddGlyph(int character, const psGlyph& glyph)
{
  _glyphs.Insert(character, glyph);
}
void psTexFont::AddKerning(int prev, int character, float kerning)
{
  _kerning.Insert(uint64_t(character) | (uint64_t(prev) << 32), kerning);
}

uint8_t psTexFont::AddTexture(psTex* tex){ tex->Grab(); _textures.Insert(tex, _textures.Capacity()); return _textures.Capacity()-1; }
void psTexFont::DestroyThis(){ delete this; }

psTexFont* psTexFont::CreateTexFont(psTex* tex, float lineheight, float ascender, float descender)
{
  return new psTexFont(tex, lineheight, ascender, descender);
}

psTexFont::psTexFont(const psTexFont& copy) : _glyphs(copy._glyphs)
{
  for(int i = 0; i < copy._textures.Capacity(); ++i)
  {
    copy._textures[i]->Grab();
    _textures.Insert(copy._textures[i], _textures.Capacity());
  }
}
psTexFont::psTexFont(psTexFont&& mov) : _textures(std::move(mov._textures)), _glyphs(std::move(mov._glyphs)), _fontlineheight(mov._fontlineheight),
  _fontascender(mov._fontascender), _fontdescender(mov._fontdescender)
{}
psTexFont::psTexFont(psTex* tex, float lineheight, float ascender, float descender) : _textures(1), _fontlineheight(lineheight), _fontascender(ascender),
  _fontdescender(descender)
{ 
  _textures[0] = tex;
  if(_textures[0])
    _textures[0]->Grab();
  Grab();
}
psTexFont::psTexFont() : _fontlineheight(0), _fontascender(0), _fontdescender(0) { Grab(); }
psTexFont::~psTexFont() { for(uint32_t i = 0; i < _textures.Capacity(); ++i) _textures[i]->Drop(); }