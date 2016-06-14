// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psFont.h"
#include "psTex.h"
#include "psColor.h"
#include "psEngine.h"
#include "bss-util/cStr.h"
#include "bss-util/os.h"
#include "freetype/ft2build.h"
#include FT_FREETYPE_H
#include "freetype/freetype.h"
#include "freetype/ftlcdfil.h"
#include <Shlobj.h>
#include <algorithm>

#ifdef BSS_COMPILER_MSC
#if defined(BSS_DEBUG) && defined(BSS_CPU_x86_64)
#pragma comment(lib, "../lib/freetype_d.lib")
#elif defined(BSS_CPU_x86_64)
#pragma comment(lib, "../lib/freetype.lib")
#elif defined(BSS_DEBUG)
#pragma comment(lib, "../lib/freetype32_d.lib")
#else
#pragma comment(lib, "../lib/freetype32.lib")
#endif
#endif

using namespace planeshader;

FT_Library psFont::PTRLIB = 0;
bss_util::cHash<const char*, psFont*, true> psFont::_Fonts; //Hashlist of all fonts, done by file.

psFont::psFont(const char* file, int psize, FONT_ANTIALIAS antialias, int dpi) : _path(file), _pointsize(psize), _curtex(0),
_curpos(VEC_ZERO), _ft2face(0), _buf(0), _dpi(!dpi ? psGUIManager::BASE_DPI : dpi), _haskerning(false)
{
  if(!PTRLIB) FT_Init_FreeType(&PTRLIB);

  float scale = _dpi / (float)psGUIManager::BASE_DPI;
  _textures.Insert(new psTex(psVeciu(bss_util::fFastRound(psize * 8 * scale)), FMT_R8G8B8A8, USAGE_RENDERTARGET, 0, 0, psVeciu(_dpi)), 0);
  _staging.Insert(new psTex(psVeciu(bss_util::fFastRound(psize * 8 * scale)), FMT_R8G8B8A8, USAGE_STAGING, 1, 0, psVeciu(_dpi)), 0);

  if(!bss_util::FileExists(_path)) //we only adjust the path if our current path doesn't exist
  {
    _path.ReplaceChar('\\', '/');
    const char* file = strrchr(_path, '/');
    if(!file || !(*file)) file = _path;
    else
      file += 1;

    wchar_t buf[MAX_PATH];
    HRESULT res = SHGetFolderPathW(0, CSIDL_FONTS, 0, SHGFP_TYPE_CURRENT, buf);
    if(res == E_FAIL) return;

    //const wchar_t* ext=wcsrchr(_path,'.'); //This doesn't work for some reason
    //if(!ext) //no extension, so the app wants us to find it
    //{
    //  WIN32_FIND_DATAW filedat;
    //  if(FindFirstFileW(cStrWF(L"%s.*", file),&filedat)!=INVALID_HANDLE_VALUE)
    //    _path=cStrWF(L"%s%s",buf,(const wchar_t*)filedat.cFileName);
    //  const wchar_t* nothing=filedat.cFileName;
    //}
    //else
    _path = cStrF("%s\\%s", cStr(buf).c_str(), file);
  }

  _adjustantialias(antialias);
  _loadfont();
}

psFont::~psFont()
{
  _cleanupfont();
  _Fonts.Remove(_hash);
}

uint16_t psFont::PreloadGlyphs(const char* glyphs)
{
  return PreloadGlyphs(cStrT<int>(glyphs).c_str());
}

uint16_t psFont::PreloadGlyphs(const int* glyphs)
{
  const uint32_t* text = (const uint32_t*)glyphs;
  uint16_t retval = 0;
  psGlyph* g;
  for(; *text; ++text)
  {
    g = _loadglyph(*text);
    retval += (g != 0 && g->texnum != -1);
  }
  return retval;
}

psFont* psFont::Create(const char* file, int psize, FONT_ANTIALIAS antialias, int dpi)
{
  if(!_driver) return 0;
  cStr str(cStrF("%s|%i|%i|%i", file, psize, antialias, dpi));
  psFont* r = _Fonts[str];
  if(r != 0) return r;
  r = new psFont(file, psize, antialias, dpi);
  r->_hash = str;
  _Fonts.Insert(r->_hash, r);
  return r;
}

void psFont::_adjustantialias(FONT_ANTIALIAS antialias)
{
  switch(antialias)
  {
  default:
  case FAA_NONE:
    _antialiased = FT_LOAD_TARGET_MONO;
    break;
  case FAA_ANTIALIAS:
    _antialiased = FT_LOAD_TARGET_NORMAL;
    break;
  case FAA_LCD:
    _antialiased = FT_LOAD_TARGET_LCD;
    break;
  case FAA_LCD_V:
    _antialiased = FT_LOAD_TARGET_LCD_V;
    break;
  case FAA_LIGHT:
    _antialiased = FT_LOAD_TARGET_LIGHT;
    break;
  }
  _enforceantialias();
}

psFont::FONT_ANTIALIAS psFont::GetAntialias() const
{
  switch(_antialiased)
  {
  default:
  case FT_LOAD_TARGET_MONO: return FAA_NONE;
  case FT_LOAD_TARGET_NORMAL: return FAA_ANTIALIAS;
  case FT_LOAD_TARGET_LCD: return FAA_LCD;
  case FT_LOAD_TARGET_LCD_V: return FAA_LCD_V;
  case FT_LOAD_TARGET_LIGHT: return FAA_LIGHT;
  }
}

void psFont::_enforceantialias()
{
  switch(_antialiased)
  {
  case FT_LOAD_TARGET_LCD:
  case FT_LOAD_TARGET_LCD_V:
    FT_Library_SetLcdFilter(PTRLIB, FT_LCD_FILTER_LIGHT);
    break;
  default:
    FT_Library_SetLcdFilter(PTRLIB, FT_LCD_FILTER_NONE);
    break;
  }
}

void psFont::_cleanupfont()
{
  if(_ft2face) FT_Done_Face(_ft2face);
  _ft2face = 0;
  if(_buf) delete[] _buf;
  _buf = 0;
}

void psFont::_stage()
{
  for(uint8_t i = 0; i < _staging.Capacity(); ++i)
    _driver->CopyTextureRect(0, psVeciu(0, 0), _staging[i]->GetRes(), _textures[i]->GetRes());
}

void psFont::_loadfont()
{
  const float FT_COEF = (1.0f / 64.0f);
  _curtex = 0;
  _curpos = psVeciu(1, 1);
  _nexty = 0;
  FILE* f;
  WFOPEN(f, cStrW(_path), L"rb"); //load font into memory
  if(!f)
  {
    PSLOG(2) << "Font face " << _path << " does not exist." << std::endl;
    return;
  }
  fseek(f, 0, SEEK_END);
  long length = ftell(f);
  fseek(f, 0, SEEK_SET);
  if(_buf) delete[] _buf;
  _buf = new uint8_t[length];
  fread(_buf, 1, length, f);
  fclose(f);

  FT_Error err; //use FT to load face from font
  if((err = FT_New_Memory_Face(PTRLIB, _buf, length, 0, &_ft2face)) != 0 || !_ft2face)
  {
    PSLOG(2) << "Font face " << _path << " failed to be created (" << err << ")" << std::endl;
    return;
  }

  if(!_ft2face->charmap)
  {
    PSLOG(2) << "Font face " << _path << " does not have a unicode character map." << std::endl;
    _cleanupfont();
    return;
  }

  FT_Pos psize = FT_F26Dot6(_pointsize * 64);
  if(FT_Set_Char_Size(_ft2face, psize, psize, _dpi, _dpi) != 0)
  { //certain fonts can only be rendered at specific sizes, so we iterate through them until we hit the closest one and try to use that
    int bestdif = 0x7FFFFFFF;
    int cur = 0;
    for(int i = 0; i < _ft2face->num_fixed_sizes; ++i)
    {
      cur = abs(_ft2face->available_sizes[i].size - psize);
      if(cur<bestdif) bestdif = cur;
    }
    if(FT_Set_Char_Size(_ft2face, 0, cur, 0, 0) != 0)
    {
      PSLOG(2) << "Font face " << _path << " can't be rendered at size " << _pointsize << std::endl;
      _cleanupfont();
      return;
    }
  }

  float invdpiscale = (_dpi == psGUIManager::BASE_DPI ? 1.0f : (psGUIManager::BASE_DPI / (float)_dpi)); // y-axis DPI scaling

  if(_ft2face->face_flags & FT_FACE_FLAG_SCALABLE) //now account for scalability 
  {
    //float x_scale = d_fontFace->size->metrics.x_scale * FT_POS_COEF * (1.0/65536.0);
    float y_scale = _ft2face->size->metrics.y_scale * (1.0f / 65536.0f);
    _fontascender = floor(_ft2face->ascender * y_scale * FT_COEF * invdpiscale);
    _fontdescender = floor(_ft2face->descender * y_scale * FT_COEF);
    _fontlineheight = floor(_ft2face->height * y_scale * FT_COEF);
  }
  else
  {
    _fontascender = floor(_ft2face->size->metrics.ascender * FT_COEF * invdpiscale);
    _fontdescender = floor(_ft2face->size->metrics.descender * FT_COEF * invdpiscale);
    _fontlineheight = floor(_ft2face->size->metrics.height * FT_COEF * invdpiscale);
  }


  _haskerning = FT_HAS_KERNING(_ft2face) != 0;
}

psGlyph* psFont::_loadglyph(uint32_t codepoint)
{
  psGlyph* retval = _glyphs[codepoint];
  if(retval != 0)
    return retval;

  psGlyph g = { RECT_ZERO, 0, 0, 0, (uint8_t)-1 };
  _glyphs.Insert(codepoint, g);
  return _renderglyph(codepoint);
}

float psFont::_loadkerning(uint32_t prev, uint32_t cur)
{
  if(!_haskerning)
    return 0.0f;

  FT_Vector kerning;
  FT_Get_Kerning(_ft2face, prev, cur, FT_KERNING_DEFAULT, &kerning);
  return kerning.x * (1.0f / 64.0f); // this would return .y for vertical layouts
}
psGlyph* psFont::_renderglyph(uint32_t codepoint)
{
  psGlyph* retval = _glyphs[codepoint];
  if(!retval) return 0;

  _enforceantialias();
  if(FT_Load_Char(_ft2face, codepoint, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT | _antialiased) != 0) //if this throws an error, remove it as a possible renderable codepoint
  {
    PSLOG(1) << "Codepoint " << codepoint << " in font " << _path << " failed to load" << std::endl;
    return retval;
  }

  FT_Bitmap& gbmp = _ft2face->glyph->bitmap;
  const float FT_COEF = (1.0f / 64.0f);
  uint32_t width = (gbmp.pixel_mode == FT_PIXEL_MODE_LCD) ? (gbmp.width / 3) : gbmp.width;
  uint32_t height = gbmp.rows;
  if(_curpos.x + width + 1>_staging[_curtex]->GetRawDim().x) //if true we ran past the edge (+1 for one pixel buffer on edges
  {
    _curpos.x = 1;
    _curpos.y = _nexty;
  }
  if(_curpos.y + height + 1>_staging[_curtex]->GetRawDim().y) //if true we ran off the bottom of the texture, so we attempt a resize
  {
    psVeciu ndim = _staging[_curtex]->GetRawDim() * 2u;
    if(!_textures[_curtex]->Resize(ndim, psTex::RESIZE_CLIP) ||
      !_staging[_curtex]->Resize(ndim, psTex::RESIZE_CLIP)) //if this fails we make a new texture
    {
      if(!_textures[_curtex]->Resize(ndim, psTex::RESIZE_DISCARD) ||
        !_staging[_curtex]->Resize(ndim, psTex::RESIZE_DISCARD)) //first we try to just replace our old one with a bigger texture.
      {
        //TODO implement multiple texture buffers
      }
      else
      { // Success, so we trigger a re-render of all codepoints
        for(auto it = _glyphs.begin(); it != _glyphs.end(); ++it)
          _renderglyph(_glyphs.GetKey(*it));
        return retval; // that re-render included us, so return.
      }
    }
    else
    {
      for(auto it = _glyphs.begin(); it != _glyphs.end(); ++it)
      {
        _glyphs.GetValue(*it)->uv.topleft *= psVec(0.5f);
        _glyphs.GetValue(*it)->uv.bottomright *= psVec(0.5f);
      }
    }
  }

  uint32_t lockpitch;
  void* lockbytes = _staging[_curtex]->Lock(lockpitch, _curpos, LOCK_WRITE);
  //lockbytes = _staging[_curtex]->LockRect(0, &psRectl(_curpos.x, _curpos.y, _curpos.x+width, _curpos.y+height), D3DLOCK_NOOVERWRITE);

  if(!lockbytes) return retval;

  psVec invdpiscale(_dpi == psGUIManager::BASE_DPI ? 1.0f : (psGUIManager::BASE_DPI / (float)_dpi));
  psVec dim = _staging[_curtex]->GetRawDim();
  retval->uv = psRect(_curpos.x / dim.x, _curpos.y / dim.y, (_curpos.x + width) / dim.x, (_curpos.y + height) / dim.y);
  retval->advance = (_ft2face->glyph->advance.x * FT_COEF * invdpiscale.x);
  retval->bearingX = (_ft2face->glyph->metrics.horiBearingX * FT_COEF * invdpiscale.x);
  retval->bearingY = (_ft2face->glyph->metrics.horiBearingY * FT_COEF * invdpiscale.y);
  retval->width = (float)width;

  _curpos.x += width + 1; //one pixel buffer
  if(_nexty<_curpos.y + height) _nexty = _curpos.y + height + 1; //one pixel buffer

  switch(gbmp.pixel_mode) //Now we render the glyph to our next available buffer
  {
  case FT_PIXEL_MODE_LCD:
    for(uint32_t i = 0; i < gbmp.rows; ++i)
    {
      uint8_t *src = gbmp.buffer + (i * gbmp.pitch);
      uint8_t *dst = reinterpret_cast<uint8_t*>(lockbytes);
      for(uint32_t j = 0; j < width; ++j) // RGBA
      {
        psColor32 c(0xFF, src[0], src[1], src[2]);
        c.WriteFormat(_staging[_curtex]->GetFormat(), dst);
        src += 3;
        dst += 4;
      }
      *((uint8_t**)&lockbytes) += lockpitch;
    }

    break;
  case FT_PIXEL_MODE_GRAY:
    for(uint32_t i = 0; i < gbmp.rows; ++i)
    {
      uint8_t *src = gbmp.buffer + (i * gbmp.pitch);
      uint8_t *dst = reinterpret_cast<uint8_t*>(lockbytes);
      for(uint32_t j = 0; j < gbmp.width; ++j) // RGBA
      {
        *dst++ = 0xFF;
        *dst++ = 0xFF;
        *dst++ = 0xFF;
        *dst++ = *src++;
      }
      *((uint8_t**)&lockbytes) += lockpitch;
    }
    break;
  case FT_PIXEL_MODE_MONO:
    for(uint32_t i = 0; i < gbmp.rows; ++i)
    {
      uint8_t *src = gbmp.buffer + (i * gbmp.pitch);
      uint32_t *dst = reinterpret_cast<uint32_t*>(lockbytes);

      for(uint32_t j = 0; j < gbmp.width; ++j)
        dst[j] = (src[j / 8] & (0x80 >> (j & 7))) ? 0xFFFFFFFF : 0x00000000;
      *((uint8_t**)&lockbytes) += lockpitch;
    }
    break;
  default:
    _staging[_curtex]->Unlock(0);
    return retval;
  }

  retval->texnum = _curtex;
  _staging[_curtex]->Unlock(0);
  _stage();
  return retval;
}
std::pair<size_t, psVec> psFont::GetIndex(const int* text, float lineheight, float letterspacing, psVec pos, std::pair<size_t, psVec> cache)
{
  cache.first = 0;
  cache.second = VEC_ZERO;
  if(!text) return cache;

  while(cache.second.y + lineheight < pos.y && text[cache.first] != 0)
  {
    if(text[cache.first] == '\n')
      cache.second.y += lineheight;
    ++cache.first;
  }

  psGlyph* g = _glyphs[text[cache.first]];
  while(text[cache.first] != 0 && (!g || cache.second.x + g->bearingX + g->width*0.5f < pos.x))
  {
    cache.second.x += g->advance + letterspacing;
    g = _glyphs[text[++cache.first]];
  }

  return cache;
}
std::pair<size_t, psVec> psFont::GetPos(const int* text, float lineheight, float letterspacing, size_t index, std::pair<size_t, psVec> cache)
{
  cache.first = index;
  cache.second = VEC_ZERO;
  if(!text) return cache;
  size_t curline = 0;

  for(size_t i = 0; i < index && text[i] != 0; ++i)
  {
    if(text[i] == '\n')
    {
      curline = i + 1;
      cache.second.y += lineheight;
    }
  }

  psGlyph* g = 0;
  for(size_t i = curline; i < index && text[i] != 0; ++i)
  {
    g = _glyphs[text[i]];
    if(!g)
      continue;
    cache.second.x += g->advance + letterspacing;
  }
  if(g) // provided the index we're on exists, replace the advance amount with the actual glyph width.
    cache.second.x = cache.second.x - g->advance - letterspacing + g->bearingX + g->width;

  return cache;
}
