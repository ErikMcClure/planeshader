// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#include "psTexFont.h"
#include "psTex.h"
#include "bss-util/Str.h"

using namespace planeshader;

psVec psTexFont::DrawText(psShader* shader, const psStateblock* stateblock, const int* text, float lineheight, float letterspacing, const psRectRotateZ& prearea, uint32_t color, psFlag flags, DELEGATE d)
{
  if(!text) return VEC_ZERO;
  float curwidth = 0.0f;
  psVec pen;
  const psGlyph* g;
  const int* pos;
  const int* peek = text;
  psRect area(prearea);
  psVec maxdim = area.Dim();
  psVec texdim;
  psVec dim(0, 0);
  uint8_t svar=_textures.Capacity();
  psRectRotateZ rect(0, 0, 0, 0, 0, VEC_ZERO, prearea.z);

  if((flags&PSFONT_BOTTOM || flags&PSFONT_VCENTER || flags&PSFONT_CLIP) ||
    (flags&PSFONT_RIGHT || flags&PSFONT_CENTER || flags&PSFONT_CLIP))
  {
    dim = maxdim;
    CalcTextDim(peek, dim, lineheight, letterspacing, flags);
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

  for(uint8_t i = 0; i < svar; ++i)
  {
    pos = text;
    peek = text;
    pen = { 0, (lineheight / _fontlineheight) * _fontascender }; // calculate starting baseline
    psVec topleft;

    if(flags&PSFONT_BOTTOM)
      topleft.y = area.bottom - dim.y;
    else if(flags&PSFONT_VCENTER)
      topleft.y = area.top + (maxdim.y - dim.y)*0.5f;
    else
      topleft.y = area.top;

    curwidth = 0.0f;
    _driver->SetTextures(reinterpret_cast<psTex**>(&_textures[i]), 1);
    psBatchObj* obj = _driver->DrawRectBatchBegin(shader, stateblock, 1, flags);
    texdim = _textures[i]->GetDim();

    topleft.x = area.left;
    psRect box;
    int last = 0;
    float lastadvance = 0;
    while(*pos)
    {
      ptrdiff_t ipos = pos - text;
      bool dobreak = false;
      g = _getchar(pos++, maxdim.x, flags, lineheight, letterspacing, pen, box, last, lastadvance, dobreak);
      if(!g || g->texnum != i) continue;

      float gdimx = (g->uv.right - g->uv.left)*texdim.x;
      float gdimy = (g->uv.bottom - g->uv.top)*texdim.y;

      rect.left = topleft.x + pen.x + g->bearingX;
      rect.top = topleft.y + pen.y - g->bearingY;

      //if(flags&PSFONT_RTL) // implement right to left rendering by flipping over the middle axis
      //{
      //  rect.left = linewidth - (rect.left - linestart) - gdimx;
      //}
      if(!d.IsEmpty()) d(ipos, rect, color);
      if(flags&PSFONT_PIXELSNAP)
      {
        rect.left = (float)bss::fFastRound(rect.left);
        rect.top = (float)bss::fFastRound(rect.top);
        rect.right = rect.left + gdimx;
        rect.bottom = rect.top + gdimy;
        _driver->DrawRectBatch(obj, rect, &g->uv, color);
      }
      else
      {
        psRect uv = g->uv;
        psRect old = { rect.left, rect.top, rect.left + gdimx, rect.top + gdimy };
        rect.left = floor(old.left);
        rect.top = floor(old.top);
        rect.right = ceil(old.right);
        rect.bottom = ceil(old.bottom);
        uv.left += (rect.left - old.left) / texdim.x;
        uv.top += (rect.top - old.top) / texdim.y;
        uv.right += (rect.right - old.right) / texdim.x;
        uv.bottom += (rect.bottom - old.bottom) / texdim.y;
        _driver->DrawRectBatch(obj, rect, &uv, color);
      }

    }
    /*while(*pos)
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
          rect.left = (float)bss::fFastRound(rect.left);
          rect.top = (float)bss::fFastRound(rect.top);
        }

        rect.right = rect.left + gdimx;
        rect.bottom = rect.top + gdimy;

        if(!d.IsEmpty()) d(ipos, rect, color);
        _driver->DrawRectBatch(obj, rect, &g->uv, color);

        pen.x += g->advance + letterspacing;
      }
      pen.y += lineheight;
    }*/
  }

  if(flags&PSFONT_CLIP)
    _driver->PopClipRect();

  return dim;
}

psVec psTexFont::DrawText(psShader* shader, const psStateblock* stateblock, const char* text, float lineheight, float letterspacing, const psRectRotateZ& area, uint32_t color, psFlag flags, DELEGATE d)
{
  size_t len = strlen(text)+1;
  if(len < 100000)
  {
    VARARRAY(int, txt, len);
    UTF8toUTF32(text, len, txt, len);
    return DrawText(shader, stateblock, txt, lineheight, letterspacing, area, color, flags, d);
  }
  bss::StrT<int> txt(text);
  return DrawText(shader, stateblock, txt, lineheight, letterspacing, area, color, flags, d);
}
bool psTexFont::_isspace(int c) // We have to make our own isspace implementation because the standard isspace() explodes if you feed it unicode characters.
{
  return c == ' ' || c == '\t' ||c == '\n' ||c == '\r' ||c == '\v' ||c == '\f';
}

psGlyph* psTexFont::_getchar(const int* text, float maxwidth, psFlag flags, float lineheight, float letterspacing, psVec& cursor, psRect& box, int& last, float& lastadvance, bool& dobreak)
{
  cursor.x += lastadvance;
  int c = *text;
  psGlyph* g = _glyphs[c];
  if(!g && c != '\n' && c != '\r')
  {
    g = _loadglyph(c);
    if(!g)
    {
      lastadvance = 0;
      last = c;
      return 0; // Note: Bad glyphs usually just have 0 width, so we don't have to check for them.
    }
  }
  float advance = 0.0f;
  box.topleft = cursor;
  box.bottomright = cursor;

  if(c != '\n' && c != '\r')
  {
    advance = g->advance + letterspacing + _getkerning(last, c);
    box.left += g->bearingX;
    box.top -= g->bearingY;
    box.right = box.left + g->width;
    box.bottom = box.top + g->height;
  }

  dobreak = c == '\n';
  if(!dobreak && flags&(PSFONT_CHARBREAK|PSFONT_WORDBREAK) && maxwidth >= 0.0f && box.right > maxwidth)
    dobreak = true;
  if(!dobreak && flags&PSFONT_WORDBREAK && _isspace(last) && !_isspace(c) && maxwidth >= 0.0f)
  {
    float right = cursor.x + advance;
    const int* cur = ++text; // we can increment cur by one, because if our current character had been over the end, it would have been handled above.
    while(*cur != 0 && !isspace(*cur))
    {
      psGlyph* gword = _glyphs[*cur];
      if(gword)
      {
        if(right + gword->bearingX + gword->width > maxwidth)
        {
          dobreak = true;
          break;
        }
        right += gword->advance + letterspacing + _getkerning(cur[-1], cur[0]); // cur[-1] is safe here because we incremented cur before entering this loop.
      }
      ++cur;
    }
  }

  if(dobreak)
  {
    box.left -= cursor.x;
    box.right -= cursor.x;
    box.top += lineheight;
    box.bottom += lineheight;
    cursor.x = 0;
    cursor.y += lineheight;
  }

  lastadvance = advance;
  last = c;
  return g;
}
float psTexFont::_getlinewidth(const int*& text, float maxwidth, psFlag flags, float letterspacing, float& cur)
{
  bool dobreak = false;
  int last = 0;
  float lastadvance = 0;
  psRect box = { 0, 0, 0, 0 };
  psVec cursor = { 0,0 };
  float width = 0.0f;
  while(*text != 0 && !dobreak)
    _getchar(text++, maxwidth, flags, 0.0f, letterspacing, cursor, box, last, lastadvance, dobreak);
  return box.right;
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
  if(!prev || c == '\n' || c == '\r' || prev == '\n' && prev == '\r') return 0;
  uint64_t key = uint64_t(c) | (uint64_t(prev) << 32);
  khiter_t iter = _kerning.Iterator(key);
  if(!_kerning.ExistsIter(iter))
  {
    _kerning.Insert(key, _loadkerning(prev, c));
    iter = _kerning.Iterator(key);
  }
  assert(_kerning.ExistsIter(iter));
  return *_kerning.GetValue(iter);
}

void psTexFont::CalcTextDim(const int* text, psVec& dest, float lineheight, float letterspacing, psFlag flags)
{
  psVec maxdim = dest;
  dest.x = 0;
  dest.y = 0;
  bool dobreak = false;
  int last = 0;
  float lastadvance = 0;
  psRect box = { 0, 0, 0, 0 };
  psVec cursor = { 0, !_fontlineheight ? 0 : ((lineheight / _fontlineheight) * _fontascender) };

  float width = 0.0f;
  while(*text != 0)
  {
    _getchar(text++, maxdim.x, flags, lineheight, letterspacing, cursor, box, last, lastadvance, dobreak);
    if(box.right > dest.x)
      dest.x = box.right;
  }
  dest.y = cursor.y - _fontdescender;
}

void psTexFont::AddGlyph(int character, const psGlyph& glyph)
{
  _glyphs.Insert(character, glyph);
}
void psTexFont::AddKerning(int prev, int character, float kerning)
{
  _kerning.Insert(uint64_t(character) | (uint64_t(prev) << 32), kerning);
}

uint8_t psTexFont::AddTexture(psTex* tex){ _textures.Insert(tex, _textures.Capacity()); return _textures.Capacity()-1; }
void psTexFont::DestroyThis(){ delete this; }

psTexFont* psTexFont::CreateTexFont(psTex* tex, float lineheight, float ascender, float descender)
{
  return new psTexFont(tex, lineheight, ascender, descender);
}

psTexFont::psTexFont(const psTexFont& copy) : _glyphs(copy._glyphs)
{
  for(int i = 0; i < copy._textures.Capacity(); ++i)
    _textures.Insert(copy._textures[i], _textures.Capacity());
}
psTexFont::psTexFont(psTexFont&& mov) : _textures(std::move(mov._textures)), _glyphs(std::move(mov._glyphs)), _fontlineheight(mov._fontlineheight),
  _fontascender(mov._fontascender), _fontdescender(mov._fontdescender)
{}
psTexFont::psTexFont(psTex* tex, float lineheight, float ascender, float descender) : _textures(1), _fontlineheight(lineheight), _fontascender(ascender),
  _fontdescender(descender)
{ 
  _textures[0] = tex;
  Grab();
}
psTexFont::psTexFont() : _fontlineheight(0), _fontascender(0), _fontdescender(0) { Grab(); }
psTexFont::~psTexFont() { }

std::pair<size_t, psVec> psTexFont::GetIndex(const int* text, float maxwidth, psFlag flags, float lineheight, float letterspacing, psVec pos)
{
  std::pair<size_t, psVec> cache = { 0, { 0, 0 } };
  if(!text) return cache;
  bool dobreak = false;
  int last = 0;
  float lastadvance = 0;
  psRect box = { 0, 0, 0, 0 };
  psGlyph* g = 0;
  for(cache.first = 0; text[cache.first] != 0; ++cache.first)
  {
    std::pair<size_t, psVec> lastcache = cache;
    lastcache.second.x += lastadvance;
    g = _getchar(text + cache.first, maxwidth, flags, lineheight, letterspacing, cache.second, box, last, lastadvance, dobreak);
    if(pos.y <= cache.second.y + lineheight && pos.x < cache.second.x + (g ? g->bearingX + (g->width*0.5f) : 0.0f))
      return cache; // we immediately terminate and return, WITHOUT adding the lastadvance on.
    if(pos.y <= cache.second.y) // Too far! return our previous cache.
      return lastcache;
  }
  cache.second.x += lastadvance; // We have to add the lastadvance on here because we left the loop at the end of the string.
  return cache;
}
std::pair<size_t, psVec> psTexFont::GetPos(const int* text, float maxwidth, psFlag flags, float lineheight, float letterspacing, size_t index)
{
  std::pair<size_t, psVec> cache = { 0, { 0, 0 } };
  if(!text) return cache;
  bool dobreak = false;
  int last = 0;
  float lastadvance = 0;
  psRect box = { 0, 0, 0, 0 };
  for(cache.first = 0; cache.first < index && text[cache.first] != 0; ++cache.first)
    _getchar(text + cache.first, maxwidth, flags, lineheight, letterspacing, cache.second, box, last, lastadvance, dobreak);
  cache.second.x += lastadvance;
  return cache;
}
