// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psFont.h"
#include "psTex.h"
#include "psEngine.h"
#include "bss-util/cStr.h"
#include "bss-util/os.h"
#include "freetype/ft2build.h"
#include FT_FREETYPE_H
#include "freetype/freetype.h"
#include "freetype/ftlcdfil.h"
#include <Shlobj.h>
#include <algorithm>

#if defined(BSS_DEBUG) && defined(BSS_CPU_x86_64)
#pragma comment(lib, "../lib/freetype64_d.lib")
#elif defined(BSS_CPU_x86_64)
#pragma comment(lib, "../lib/freetype64.lib")
#elif defined(BSS_DEBUG)
#pragma comment(lib, "../lib/freetype_d.lib")
#else
#pragma comment(lib, "../lib/freetype.lib")
#endif

using namespace planeshader;

FT_Library psFont::PTRLIB=0;
bss_util::cHash<const char*, psFont*, true> psFont::_Fonts; //Hashlist of all fonts, done by file.

psFont::psFont(const char* file, int psize, float lineheight, FONT_ANTIALIAS antialias) : psTexFont(0, lineheight), _path(file), _pointsize(psize), _curtex(0),
  _curpos(VEC_ZERO), _ft2face(0), _buf(0)
{
  if(!PTRLIB) FT_Init_FreeType(&PTRLIB);

  _textures.Insert(new psTex(psVeciu(psize*8, psize*8), FMT_A8R8G8B8, USAGE_AUTOGENMIPMAP|USAGE_DYNAMIC, 0), 0);

  if(!bss_util::FileExists(_path)) //we only adjust the path if our current path doesn't exist
  {
    _path.ReplaceChar('\\', '/');
    const char* file = strrchr(_path, '/');
    if(!file || !(*file)) file=_path;
    else
      file+=1;

    wchar_t buf[MAX_PATH];
    HRESULT res = SHGetFolderPathW(0, CSIDL_FONTS, 0, SHGFP_TYPE_CURRENT, buf);
    if(res==E_FAIL) return;

    //const wchar_t* ext=wcsrchr(_path,'.'); //This doesn't work for some reason
    //if(!ext) //no extension, so the app wants us to find it
    //{
    //  WIN32_FIND_DATAW filedat;
    //  if(FindFirstFileW(cStrWF(L"%s.*", file),&filedat)!=INVALID_HANDLE_VALUE)
    //    _path=cStrWF(L"%s%s",buf,(const wchar_t*)filedat.cFileName);
    //  const wchar_t* nothing=filedat.cFileName;
    //}
    //else
    _path=cStrF("%s\\%s", cStr(buf).c_str(), file);
  }

  _adjustantialias(antialias);
  _loadfont();
}

psFont::~psFont()
{
  _cleanupfont();
  _Fonts.Remove(_hash);
}

unsigned short psFont::PreloadGlyphs(const char* glyphs)
{
  return PreloadGlyphs(cStrT<int>(glyphs).c_str());
}

unsigned short psFont::PreloadGlyphs(const int* glyphs)
{
  const unsigned int* text=(const unsigned int*)glyphs;
  unsigned short retval=0;
  psGlyph* g;
  for(; *text; ++text)
  {
    g = _loadglyph(*text);
    retval += (g!=0 && g->texnum != -1);
  }
  return retval;
}

psFont* psFont::Create(const char* file, int psize, float lineheight, FONT_ANTIALIAS antialias)
{
  cStr str(cStrF("%s|%i|%i|%i", file, psize, lineheight, antialias));
  psFont* r = _Fonts[str];
  if(r!=0) return r;
  r = new psFont(file, psize, lineheight, antialias);
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
    _antialiased=FT_LOAD_TARGET_MONO;
    break;
  case FAA_ANTIALIAS:
    _antialiased=FT_LOAD_TARGET_NORMAL;
    break;
  case FAA_LCD:
    _antialiased=FT_LOAD_TARGET_LCD;
    break;
  case FAA_LCD_V:
    _antialiased=FT_LOAD_TARGET_LCD_V;
    break;
  case FAA_LIGHT:
    _antialiased=FT_LOAD_TARGET_LIGHT;
    break;
  }
  _enforceantialias();
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

void psFont::_loadfont()
{
  const float FT_COEF = (1.0f/64.0f);
  _curtex = 0;
  _curpos = VEC_ZERO;
  _nexty = 0;
  FILE* f;
  WFOPEN(f, cStrW(_path), L"rb"); //load font into memory
  if(!f) {
    PSLOG(2) << "Font face " << _path << " does not exist." << std::endl;
    return;
  }
  fseek(f, 0, SEEK_END);
  long length=ftell(f);
  fseek(f, 0, SEEK_SET);
  if(_buf) delete[] _buf;
  _buf=new unsigned char[length];
  fread(_buf, 1, length, f);
  fclose(f);

  FT_Error err; //use FT to load face from font
  if((err=FT_New_Memory_Face(PTRLIB, _buf, length, 0, &_ft2face))!=0 || !_ft2face) {
    PSLOG(2) << "Font face " << _path << " failed to be created (" << err << ")" << std::endl;
    return;
  }

  if(!_ft2face->charmap) {
    PSLOG(2) << "Font face " << _path << " does not have a unicode character map." << std::endl;
    _cleanupfont();
    return;
  }

  FT_Pos psize=FT_F26Dot6(_pointsize*64);
  if(FT_Set_Char_Size(_ft2face, psize, psize, 96, 96)!=0) //windows has auto-DPI scaling so we ignore it
  { //certain fonts can only be rendered at specific sizes, so we iterate through them until we hit the closest one and try to use that
    int bestdif=0x7FFFFFFF;
    int cur=0;
    for(int i = 0; i < _ft2face->num_fixed_sizes; ++i)
    {
      cur=abs(_ft2face->available_sizes[i].size-psize);
      if(cur<bestdif) bestdif=cur;
    }
    if(FT_Set_Char_Size(_ft2face, 0, cur, 0, 0)!=0) {
      PSLOG(2) << "Font face " << _path << " can't be rendered at size " << _pointsize << std::endl;
      _cleanupfont();
      return;
    }
  }

  //if (_ft2face->face_flags & FT_FACE_FLAG_SCALABLE) //now account for scalability 
  //{
  //    //float x_scale = d_fontFace->size->metrics.x_scale * FT_POS_COEF * (1.0/65536.0);
  //    float y_scale = _ft2face->size->metrics.y_scale * (1.0f / 65536.0f);
  //    _ascender = _ft2face->ascender * y_scale;
  //    _ascender *= (1.0f/64.0f);
  //    _descender = _ft2face->descender * y_scale;
  //    _height = _ft2face->height * y_scale;
  //}
  //else
  {
    //_ascender = _ft2face->size->metrics.ascender * FT_COEF;
    //_descender = _ft2face->size->metrics.descender * FT_COEF;
    //_height = _ft2face->size->metrics.height * FT_COEF;
  }

  if(_lineheight==0) _lineheight = _ft2face->size->metrics.height * FT_COEF;
}

psGlyph* psFont::_loadglyph(unsigned int codepoint)
{
  psGlyph* retval = _glyphs[codepoint];
  if(retval != 0)
    return retval;

  psGlyph g ={ RECT_ZERO, 0, 0, 0, -1 };
  _glyphs.Insert(codepoint, g);
  return _renderglyph(codepoint);
}

psGlyph* psFont::_renderglyph(unsigned int codepoint)
{
  psGlyph* retval = _glyphs[codepoint];
  if(!retval) return 0;

  _enforceantialias();
  if(FT_Load_Char(_ft2face, codepoint, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT | _antialiased)!=0) //if this throws an error, remove it as a possible renderable codepoint
  {
    PSLOG(1) << "Codepoint " << codepoint << " in font " << _path << " failed to load" << std::endl;
    return retval;
  }

  FT_Bitmap& gbmp = _ft2face->glyph->bitmap;
  const float FT_COEF = (1.0f/64.0f);
  unsigned int width=gbmp.pixel_mode==FT_PIXEL_MODE_LCD?gbmp.width/3:gbmp.width;
  unsigned int height=gbmp.rows;
  if(_curpos.x+width+1>_textures[_curtex]->GetDim().x) //if true we ran past the edge (+1 for one pixel buffer on edges
  {
    _curpos.x=1;
    _curpos.y=_nexty;
  }
  if(_curpos.y+height+1>_textures[_curtex]->GetDim().y) //if true we ran off the bottom of the texture, so we attempt a resize
  {
    if(!_textures[_curtex]->Resize(_textures[_curtex]->GetDim()*2u, psTex::RESIZE_CLIP)) //if this fails we make a new texture
    {
      if(!_textures[_curtex]->Resize(_textures[_curtex]->GetDim()*2u, psTex::RESIZE_DISCARD)) //first we try to just replace our old one with a bigger texture.
      {
        //TODO implement multiple texture buffers
      } 
      else 
      { // Success, so we trigger a re-render of all codepoints
        for(auto it = _glyphs.begin(); it!=_glyphs.end(); ++it)
          _renderglyph(_glyphs.GetKey(*it));
        return retval; // that re-render included us, so return.
      }
    }
    else
    {
      for(auto it = _glyphs.begin(); it!=_glyphs.end(); ++it)
      {
        _glyphs.GetValue(*it)->uv.topleft*=psVec(0.5f);
        _glyphs.GetValue(*it)->uv.bottomright*=psVec(0.5f);
      }
    }
  }

  unsigned int lockpitch;
  void* lockbytes = _textures[_curtex]->Lock(lockpitch, _curpos, LOCK_WRITE);
  //lockbytes = _textures[_curtex]->LockRect(0, &psRectl(_curpos.x, _curpos.y, _curpos.x+width, _curpos.y+height), D3DLOCK_NOOVERWRITE);

  if(!lockbytes) return retval;

  psVec dim=_textures[_curtex]->GetDim();
  retval->uv=psRect(_curpos.x/dim.x, _curpos.y/dim.y, (_curpos.x+width)/dim.x, (_curpos.y+height)/dim.y);
  retval->width=(_ft2face->glyph->metrics.horiAdvance * FT_COEF);
  retval->ascender=(-_ft2face->glyph->metrics.horiBearingY * FT_COEF);
  retval->advance=(_ft2face->glyph->metrics.horiBearingX * FT_COEF);

  _curpos.x+=width+1; //one pixel buffer
  if(_nexty<_curpos.y+height) _nexty = _curpos.y+height+1; //one pixel buffer

  switch(gbmp.pixel_mode) //Now we render the glyph to our next available buffer
  {
  case FT_PIXEL_MODE_LCD:
    for(unsigned int i = 0; i < gbmp.rows; ++i)
    {
      unsigned char *src = gbmp.buffer + (i * gbmp.pitch);
      unsigned char *dst = reinterpret_cast<unsigned char*>(lockbytes);
      for(unsigned int j = 0; j < width; ++j) // RGBA
      {
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        bss_util::rswap(dst[-1], dst[-3]);
        *dst++ = 0xFF;
        //*dst++ = src[-2]+src[-1]+src[0];
      }
      *((unsigned char**)&lockbytes)+=lockpitch;
    }

    break;
  case FT_PIXEL_MODE_GRAY:
    for(unsigned int i = 0; i < gbmp.rows; ++i)
    {
      unsigned char *src = gbmp.buffer + (i * gbmp.pitch);
      unsigned char *dst = reinterpret_cast<unsigned char*>(lockbytes);
      for(unsigned int j = 0; j < gbmp.width; ++j) // RGBA
      {
        *dst++ = 0xFF;
        *dst++ = 0xFF;
        *dst++ = 0xFF;
        *dst++ = *src++;
      }
      *((unsigned char**)&lockbytes)+=lockpitch;
    }
    break;
  case FT_PIXEL_MODE_MONO:
    for(unsigned int i = 0; i < gbmp.rows; ++i)
    {
      unsigned char *src = gbmp.buffer + (i * gbmp.pitch);
      unsigned __int32 *dst = reinterpret_cast<unsigned __int32*>(lockbytes);

      for(unsigned int j = 0; j < gbmp.width; ++j)
        dst[j] = (src[j / 8] & (0x80 >> (j & 7))) ? 0xFFFFFFFF : 0x00000000;
      *((unsigned char**)&lockbytes)+=lockpitch;
    }
    break;
  default:
    _textures[_curtex]->Unlock(0);
    return retval;
  }

  retval->texnum=_curtex;
  _textures[_curtex]->Unlock(0);
  return retval;
}