// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psTex.h"
#include "psStateblock.h"
#include "psColor.h"

using namespace planeshader;

psPixelArray::psPixelArray(void* res, FORMATS format, uint8_t lockflags, uint8_t miplevel) : _res(res), _miplevel(miplevel)
{
  _mem = reinterpret_cast<uint8_t*>(_driver->LockTexture(_res, lockflags, _rowpitch, _miplevel));
}
psPixelArray::~psPixelArray() { _driver->UnlockTexture(_res, _miplevel); }

psTex::psTex(psTex&& mov) : _res(mov._res), _view(mov._view), _dim(mov._dim), _format(mov._format), _usage(mov._usage),
  _miplevels(mov._miplevels), _texblock(std::move(mov._texblock)), _dpi(mov._dpi)
{
  mov._res=0;
  mov._view=0;
}

psTex::psTex(const psTex& copy) : _texblock(copy._texblock), _dpi(copy._dpi), _view(0)
{
  _res = _driver->CreateTexture(copy._dim, copy._format, copy._usage, copy._miplevels, 0, &_view, _texblock);
  _driver->CopyResource(_res, copy._res, psDriver::RES_TEXTURE);
  if(_res) _applydesc(_driver->GetTextureDesc(_res));
  Grab();
}

psTex::psTex(psVeciu dim, FORMATS format, uint32_t usage, uint8_t miplevels, psTexblock* texblock, psVeciu dpi) : _texblock(texblock), _dpi(dpi), _view(0)
{
  if((usage & USAGE_STAGING) != USAGE_STAGING) usage |= USAGE_SHADER_RESOURCE; // You MUST use != for USAGE_STAGING because it is NOT a flag!
  _res = _driver->CreateTexture(dim, format, usage, miplevels, 0, &_view, _texblock);
  if(_res) _applydesc(_driver->GetTextureDesc(_res));
  Grab();
}

psTex::psTex(void* res, void* view, psVeciu dim, FORMATS format, uint32_t usage, uint8_t miplevels, psTexblock* texblock, psVeciu dpi) :
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

psTex* BSS_FASTCALL psTex::Create(const char* file, uint32_t usage, FILTERS mipfilter, uint8_t miplevels, FILTERS loadfilter, bool sRGB, psTexblock* texblock, psVeciu dpi)
{
  void* view = 0;
  void* res = _driver->LoadTexture(file, usage, FMT_UNKNOWN, &view, miplevels, mipfilter, loadfilter, VEC_ZERO, texblock, sRGB);
  return _create(res, view, dpi, texblock);
}

psTex* BSS_FASTCALL psTex::Create(const void* data, uint32_t datasize, uint32_t usage, FILTERS mipfilter, uint8_t miplevels, FILTERS loadfilter, bool sRGB, psTexblock* texblock, psVeciu dpi)
{
  if(!datasize)
    return Create((const char*)data, usage);
  void* view = 0;
  void* res = _driver->LoadTextureInMemory(data, datasize, usage, FMT_UNKNOWN, &view, miplevels, mipfilter, loadfilter, VEC_ZERO, texblock, sRGB);
  return _create(res, view, dpi, texblock);
}
psTex* BSS_FASTCALL psTex::Create(const psTex& copy)
{
  return new psTex(copy);
}
psTex* BSS_FASTCALL psTex::Create(psVeciu dim, FORMATS format, uint32_t usage, uint8_t miplevels, psTexblock* texblock, psVeciu dpi)
{
  return new psTex(dim, format, usage, miplevels, texblock, dpi);
}

psTex* BSS_FASTCALL psTex::_create(void* res, void* view, psVeciu dpi, psTexblock* texblock)
{
  if(!res) return 0;
  TEXTURE_DESC desc = _driver->GetTextureDesc(res);
  return new psTex(res, view, desc.dim.xy, desc.format, desc.usage, desc.miplevels, texblock, dpi);
}

inline void* psTex::Lock(uint32_t& rowpitch, psVeciu offset, uint8_t lockflags, uint8_t miplevel)
{
  uint16_t bytes = psColor::BitsPerPixel(_format)>>3;
  void* root = _driver->LockTexture(_res, lockflags, rowpitch, miplevel);
  return ((uint8_t*)root) + offset.x*bytes + offset.y*rowpitch;
}

inline void psTex::Unlock(uint8_t miplevel)
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