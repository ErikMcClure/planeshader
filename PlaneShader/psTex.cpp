// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psTex.h"

using namespace planeshader;

psTex::psTex(psTex&& mov) : _res(mov._res), _view(mov._view), _dim(mov._dim), _format(mov._format), _usage(mov._usage),
  _miplevels(mov._miplevels)
{
  mov._res=0;
  mov._view=0;
}
psTex::psTex(const psTex& copy) : _dim(copy._dim), _format(copy._format), _usage(copy._usage),
  _miplevels(copy._miplevels)
{
  _res = _driver->CreateTexture(copy._dim, copy._format, copy._usage, copy._miplevels, 0, &_view);
}
psTex::psTex(psVeciu dim, FORMATS format, USAGETYPES usage, unsigned char miplevels)
{
  _res = _driver->CreateTexture(dim, format, usage, miplevels, 0, &_view);
}
psTex::psTex(void* res, void* view, psVeciu dim, FORMATS format, USAGETYPES usage, unsigned char miplevels) :
  _res(res), _view(view), _dim(dim), _format(format), _usage(usage), _miplevels(miplevels)
{
}
psTex::~psTex()
{
  if(_res) _driver->FreeResource(_res, psDriver::RES_TEXTURE);
  if(_view) _driver->FreeResource(_view, (_usage&USAGE_RENDERTARGET)?psDriver::RES_SURFACE:psDriver::RES_DEPTHVIEW);
}
void psTex::DestroyThis() { delete this; }
psTex& psTex::operator=(const psTex& right)
{
  psTex::~psTex();
  _dim=right._dim;
  _format=right._format;
  _usage=right._usage;
  _miplevels=right._miplevels;
  _res = _driver->CreateTexture(right._dim, right._format, right._usage, right._miplevels, 0, &_view);
  return *this;
}
psTex& psTex::operator=(psTex&& right)
{
  psTex::~psTex();
  _dim=right._dim;
  _format=right._format;
  _usage=right._usage;
  _miplevels=right._miplevels;
  _res = right._res;
  _view = right._view;
  right._res = 0;
  right._view = 0;
  return *this;
}