// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psEngine.h"
#include "psPass.h"
#include "psDirectX10.h"
#include "psNullDriver.h"
#include "psStateblock.h"
#include "bss-util/profiler.h"

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
const float psDriver::identity[4][4] ={ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

psDriver* psDriverHold::GetDriver() { return _driver; }

psEngine::psEngine(const PSINIT& init) : cLog(!init.errout?"PlaneShader.log":0, init.errout), delta(_delta), secdelta(_secdelta), _curpass(0), _passes(1), _mainpass(0), _timewarp(1.0), _secdelta(0.0)
{
  PROFILE_FUNC();
  if(_instance!=0) // If you try to make another instance, it violently explodes.
    terminate();
  _instance = this;
  fSetDenormal(false);
  fSetRounding(true);

  GetStream() << "--- PLANESHADER v." << PS_VERSION_MAJOR << '.' << PS_VERSION_MINOR << '.' << PS_VERSION_REVISION << " LOG ---" << std::endl;
  srand(GetTickCount());
  rand();

  _create(psVeciu(init.width, init.height), init.fullscreen, init.composite, 0);

  switch(init.driver)
  {
  case RealDriver::DRIVERTYPE_DX10:
    new psDirectX10(psVeciu(init.width, init.height), init.antialias, init.vsync, init.fullscreen, init.destalpha, _window);
    if(((psDirectX10*)_driver)->GetLastError()!=0) { delete _driver; _driver=0; }
    break;
  }
  if(!_driver)
  {
    PSLOGV(0, "Driver encountered a fatal error");
    Quit();
    return;
  }

  psStateblock::DEFAULT = psStateblock::Create(0, 0);
  _driver->SetStateblock(psStateblock::DEFAULT->GetSB());
  _resizewindow(_driver->rawscreendim.x, _driver->rawscreendim.y, init.fullscreen);
  _driver->SetExtent(init.nearextent, init.farextent);
  _mainpass = new psPass();
  _passes[0] = _mainpass;
}
psEngine::~psEngine()
{
  PROFILE_FUNC();
  if(_driver)
    delete _driver;
  _driver=0;

  if(_mainpass)
    delete _mainpass;
  _mainpass=0;

  _instance=0;
}
bool psEngine::Begin()
{
  PROFILE_FUNC();
  if(GetQuit())
    return false;
  //if(_flags&PSENGINE_AUTOANI)
  //  psAnimation::AniInterpolate(delta);
  if(!_driver->Begin()) //lost device (DX9 only)
    return false;
  _curpass=0;
  _passes[0]->Begin();
  return true;
}
void psEngine::End()
{
  PROFILE_FUNC();
  while(NextPass());

  //if(_realdriver.dx9->_mousehittest!=0) //if this is nonzero we need to toggle transparency based on a system wide mouse hit test
  //  if(_realdriver.dx9->MouseHitTest(GetMouseExact(), _alphacutoff))
  //    SetWindowLong(_window, GWL_EXSTYLE, ((GetWindowLong(_window, GWL_EXSTYLE))&(~WS_EX_TRANSPARENT)));
  //  else
  //    SetWindowLong(_window, GWL_EXSTYLE, ((GetWindowLong(_window, GWL_EXSTYLE))|WS_EX_TRANSPARENT));

  _driver->End();

  if(_timewarp == 1.0) //Refresh the time
    cHighPrecisionTimer::Update();
  else
    cHighPrecisionTimer::Update(_timewarp);

  //_realdelta =_delta*_timescale;
  _secdelta = _delta/1000.0;

  FlushMessages();
}
void psEngine::End(double delta)
{
  End();
}
bool psEngine::NextPass()
{
  PROFILE_FUNC();
  if(_curpass >= _passes.Size()) return false;
  _passes[_curpass]->End();
  if(++_curpass >= _passes.Size()) return false;
  _passes[_curpass]->Begin();
  return true;
}
psEngine* psEngine::Instance()
{
  return _instance;
}
bool psEngine::InsertPass(psPass& pass, unsigned short index)
{
  if(index>_passes.Size())
    return false;
  _passes.Insert(&pass, index);
  return true;
}
bool psEngine::RemovePass(unsigned short index)
{
  if(!index || index>=_passes.Size()) return false;
  _passes.Remove(index);
  return true;
}