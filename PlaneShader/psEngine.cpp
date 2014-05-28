// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psEngine.h"
#include "psPass.h"
#include "psDirectX10.h"

using namespace planeshader;
using namespace bss_util;

#ifdef BSS_COMPILER_MSC
#if defined(BSS_DEBUG) && defined(BSS_CPU_x86_64)
#pragma comment(lib, "../lib/bss-util64_d.lib")
#elif defined(BSS_CPU_x86_64)
#pragma comment(lib, "../lib/bss-util64.lib")
#elif defined(BSS_DEBUG)
#pragma comment(lib, "../lib/bss-util_d.lib")
#else
#pragma comment(lib, "../lib/bss-util.lib")
#endif
#endif

psDriver* psDriverHold::_driver=0;
psEngine* psEngine::_instance=0;

psEngine::psEngine(const PSINIT& init) : delta(_delta), secdelta(_secdelta), _curpass(0)
{
  if(_instance!=0) // If you try to make another instance, it violently explodes.
    terminate();
  _instance = this;
  fSetDenormal(false);
  fSetRounding(true);

  switch(init.driver)
  {
  case RealDriver::DRIVERTYPE_DX10:
    _driver = new psDirectX10(psVeciu(init.height,init.width));
    break;
  }
}
psEngine::~psEngine()
{
  _instance=0;
}
bool psEngine::Begin()
{
  if(GetQuit())
    return false;
  if(!_driver->Begin()) //lost device (DX9 only)
    return false;
  _curpass=0;
  _passes[0]->Begin();
  return true;
}
void psEngine::End()
{
  while((_curpass+1)<_passes.Size())
    NextPass();
  _passes[_curpass]->End();
  _driver->End();
}
void psEngine::End(double delta)
{
  End();
}
void psEngine::NextPass()
{
  _passes[_curpass]->End();
  _passes[++_curpass]->Begin();
}
psEngine* psEngine::Instance()
{
  return _instance;
}