// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psTex.h"
#include "psStateblock.h"
#include "psColor.h"

using namespace planeshader;

psTex::psTex(psTex&& mov) : _res(mov._res), _view(mov._view), _dim(mov._dim), _format(mov._format), _usage(mov._usage),
  _miplevels(mov._miplevels), _texblock(std::move(mov._texblock)), _dpi(mov._dpi)
{
  mov._res=0;
  mov._view=0;
}

psTex::psTex(const psTex& copy) : _texblock(copy._texblock), _dpi(copy._dpi)
{
  _res = _driver->CreateTexture(copy._dim, copy._format, copy._usage, copy._miplevels, 0, &_view, _texblock);
  _driver->CopyResource(_res, copy._res, psDriver::RES_TEXTURE);
  if(_res) _applydesc(_driver->GetTextureDesc(_res));
  Grab();
}

psTex::psTex(psVeciu dim, FORMATS format, unsigned int usage, unsigned char miplevels, psTexblock* texblock, psVeciu dpi) : _texblock(texblock), _dpi(dpi)
{
  if(!(usage&USAGE_STAGING)) usage |= USAGE_SHADER_RESOURCE;
  _res = _driver->CreateTexture(dim, format, usage, miplevels, 0, &_view, _texblock);
  if(_res) _applydesc(_driver->GetTextureDesc(_res));
  Grab();
}

psTex::psTex(void* res, void* view, psVeciu dim, FORMATS format, unsigned int usage, unsigned char miplevels, psTexblock* texblock, psVeciu dpi) :
  _res(res), _view(view), _dim(dim), _format(format), _usage(usage), _miplevels(miplevels), _texblock(texblock), _dpi(dpi)
{
  Grab();
}

psTex::~psTex()
{
  if(_res) _driver->FreeResource(_res, psDriver::RES_TEXTURE);
  if(_view) _driver->FreeResource(_view, (_usage&USAGE_RENDERTARGET)?psDriver::RES_SURFACE:psDriver::RES_DEPTHVIEW);
}

void psTex::DestroyThis() { delete this; }
void BSS_FASTCALL psTex::_applydesc(TEXTURE_DESC& desc)
{
  _dim = desc.dim.xy;
  _format = desc.format;
  _usage = desc.usage;
  _miplevels = desc.miplevels;
}

psTex* BSS_FASTCALL psTex::Create(const char* file, unsigned int usage, FILTERS mipfilter, unsigned char miplevels, FILTERS loadfilter, bool sRGB, psVeciu dpi)
{
  void* view = 0;
  void* res = _driver->LoadTexture(file, usage, FMT_UNKNOWN, &view, miplevels, mipfilter, loadfilter, VEC_ZERO, 0, sRGB);
  return _create(res, view, dpi);
}

psTex* BSS_FASTCALL psTex::Create(const void* data, unsigned int datasize, unsigned int usage, FILTERS mipfilter, unsigned char miplevels, FILTERS loadfilter, bool sRGB, psVeciu dpi)
{
  if(!datasize)
    return Create((const char*)data, usage);
  void* view = 0;
  void* res = _driver->LoadTextureInMemory(data, datasize, usage, FMT_UNKNOWN, &view, miplevels, mipfilter, loadfilter, VEC_ZERO, 0, sRGB);
  return _create(res, view, dpi);
}
psTex* BSS_FASTCALL psTex::Create(const psTex& copy)
{
  return new psTex(copy);
}
psTex* BSS_FASTCALL psTex::Create(psVeciu dim, FORMATS format, unsigned int usage, unsigned char miplevels, psTexblock* texblock, psVeciu dpi)
{
  return new psTex(dim, format, usage, miplevels, texblock, dpi);
}

psTex* BSS_FASTCALL psTex::_create(void* res, void* view, psVeciu dpi)
{
  if(!res) return 0;
  TEXTURE_DESC desc = _driver->GetTextureDesc(res);
  return new psTex(res, view, desc.dim.xy, desc.format, desc.usage, desc.miplevels, 0, dpi);
}

inline void* psTex::Lock(unsigned int& rowpitch, psVeciu offset, unsigned char lockflags, unsigned char miplevel)
{
  unsigned short bytes = psColor::BitsPerPixel(_format)>>3;
  void* root = _driver->LockTexture(_res, lockflags, rowpitch, miplevel);
  return ((unsigned char*)root) + offset.x*bytes + offset.y*rowpitch;
}

inline void psTex::Unlock(unsigned char miplevel)
{
  _driver->UnlockTexture(_res, miplevel);
}

inline bool psTex::Resize(psVeciu dim, RESIZE resize)
{
  if(dim == _dim) return true; //nothing to do
  if(resize == RESIZE_STRETCH) return false; // Not supported
  void* view;
  void* res = _driver->CreateTexture(dim, _format, _usage, _miplevels, 0, &view, _texblock);
  if(!res) // If this failed then view couldn't have been created either.
    return false;

  switch(resize)
  {
  case RESIZE_CLIP:
    _driver->CopyTextureRect(&psRectiu(0, 0, bssmin(dim.x, _dim.x), bssmin(dim.y, _dim.y)), psVeciu(0, 0), _res, res);
    break;
  case RESIZE_STRETCH:
    break; // Not supported
  case RESIZE_DISCARD:
    break; // do not attempt to copy over data from the old texture into the new one, just let the old one get destroyed.
  }

  if(_res) _driver->FreeResource(_res, psDriver::RES_TEXTURE);
  if(_view) _driver->FreeResource(_view, (_usage&USAGE_RENDERTARGET)?psDriver::RES_SURFACE:psDriver::RES_DEPTHVIEW);
  _res = res;
  _view = view;
  _dim = dim;
  return true;
}


psTex& psTex::operator=(const psTex& right)
{
  psTex::~psTex();
  _dim=right._dim;
  _format=right._format;
  _usage=right._usage;
  _miplevels=right._miplevels;
  _texblock=right._texblock;
  _res = _driver->CreateTexture(right._dim, right._format, right._usage, right._miplevels, 0, &_view, _texblock);
  _driver->CopyResource(_res, right._res, psDriver::RES_TEXTURE);
  return *this;
}

psTex& psTex::operator=(psTex&& right)
{
  psTex::~psTex();
  _dim=right._dim;
  _format=right._format;
  _usage=right._usage;
  _miplevels=right._miplevels;
  _texblock=std::move(right._texblock);
  _res = right._res;
  _view = right._view;
  right._res = 0;
  right._view = 0;
  return *this;
}