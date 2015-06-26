// Copyright �2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psTex.h"
#include "psStateblock.h"

using namespace planeshader;

psTex::psTex(psTex&& mov) : _res(mov._res), _view(mov._view), _dim(mov._dim), _format(mov._format), _usage(mov._usage),
  _miplevels(mov._miplevels), _texblock(mov._texblock)
{
  mov._res=0;
  mov._view=0;
  mov._texblock=0;
}

psTex::psTex(const psTex& copy) : _texblock(copy._texblock)
{
  if(_texblock) _texblock->Grab();
  _res = _driver->CreateTexture(copy._dim, copy._format, copy._usage, copy._miplevels, 0, &_view, _texblock);
  _applydesc(_driver->GetTextureDesc(_res));
}

psTex::psTex(psVeciu dim, FORMATS format, USAGETYPES usage, unsigned char miplevels, psTexblock* texblock) : _texblock(texblock)
{
  if(_texblock) _texblock->Grab();
  _res = _driver->CreateTexture(dim, format, usage, miplevels, 0, &_view, _texblock);
  _applydesc(_driver->GetTextureDesc(_res));
}

psTex::psTex(void* res, void* view, psVeciu dim, FORMATS format, USAGETYPES usage, unsigned char miplevels, psTexblock* texblock) :
  _res(res), _view(view), _dim(dim), _format(format), _usage(usage), _miplevels(miplevels), _texblock(texblock)
{
  if(_texblock) _texblock->Grab();
}

psTex::~psTex()
{
  if(_res) _driver->FreeResource(_res, psDriver::RES_TEXTURE);
  if(_view) _driver->FreeResource(_view, (_usage&USAGE_RENDERTARGET)?psDriver::RES_SURFACE:psDriver::RES_DEPTHVIEW);
  if(_texblock) _texblock->Drop();
}

void psTex::DestroyThis() { delete this; }
void BSS_FASTCALL psTex::_applydesc(TEXTURE_DESC& desc)
{
  _dim = desc.dim.xy;
  _format = desc.format;
  _usage = desc.usage;
  _miplevels = desc.miplevels;
}

psTex* BSS_FASTCALL psTex::Create(const char* file, USAGETYPES usage, FILTERS mipfilter, FILTERS loadfilter)
{
  void* view = 0;
  void* res = _driver->LoadTexture(file, usage, FMT_UNKNOWN, &view, 0, mipfilter, loadfilter);
  return _create(res, view);
}
psTex* BSS_FASTCALL psTex::Create(const void* data, unsigned int datasize, USAGETYPES usage, FILTERS mipfilter, FILTERS loadfilter)
{
  if(!datasize)
    return Create((const char*)data, usage);
  void* view = 0;
  void* res = _driver->LoadTextureInMemory(data, datasize, usage, FMT_UNKNOWN, &view, 0, mipfilter, loadfilter);
  return _create(res, view);
}

psTex* BSS_FASTCALL psTex::_create(void* res, void* view)
{
  if(!res) return 0;
  TEXTURE_DESC desc = _driver->GetTextureDesc(res);
  psTex* ret = new psTex(res, view, desc.dim.xy, desc.format, desc.usage, desc.miplevels);
  ret->Grab();
  return ret;
}

psTex& psTex::operator=(const psTex& right)
{
  psTex::~psTex();
  _dim=right._dim;
  _format=right._format;
  _usage=right._usage;
  _miplevels=right._miplevels;
  _texblock=right._texblock;
  if(_texblock) _texblock->Grab();
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
  _texblock=right._texblock;
  _res = right._res;
  _view = right._view;
  right._res = 0;
  right._view = 0;
  right._texblock = 0;
  return *this;
}