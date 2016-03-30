// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psEngine.h"
#include "psPass.h"
#include "psTex.h"
#include "psDirectX11.h"
#include "psNullDriver.h"
#include "psStateblock.h"
#include "bss-util/profiler.h"
#include "ps_feather.h"

using namespace planeshader;
using namespace bss_util;

#ifdef BSS_COMPILER_MSC
#if defined(BSS_DEBUG) && defined(BSS_CPU_x86_64)
#pragma comment(lib, "../lib/bss-util_d.lib")
#elif defined(BSS_CPU_x86_64)
#pragma comment(lib, "../lib/bss-util.lib")
#elif defined(BSS_DEBUG)
#pragma comment(lib, "../lib/bss-util32_d.lib")
#else
#pragma comment(lib, "../lib/bss-util32.lib")
#endif
#endif

psDriver* psDriverHold::_driver=0;
psEngine* psEngine::_instance=0;
const float psDriver::identity[4][4] ={ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

psDriver* psDriverHold::GetDriver() { return _driver; }

psEngine::psEngine(const PSINIT& init) : cLog(!init.errout?"PlaneShader.log":0, init.errout), delta(_delta), secdelta(_secdelta), _curpass(0), _passes(1),
  _mainpass(0), _timewarp(1.0), _secdelta(0.0), _mediapath(init.mediapath)
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

  psVeciu dim = _create(psVeciu(init.width, init.height), init.mode, 0);

  switch(init.driver)
  {
  case RealDriver::DRIVERTYPE_NULL:
    PSLOG(4) << "Initializing Null Driver" << std::endl;
    //new psNullDriver();
    break;
  case RealDriver::DRIVERTYPE_DX10:
    //PSLOG(4) << "Initializing DirectX10 Driver" << std::endl;
    //new psDirectX10(dim, init.antialias, init.vsync, init.mode == PSINIT::MODE_FULLSCREEN, _window);
    //if(((psDirectX10*)_driver)->GetLastError()!=0) { delete _driver; _driver=0; }
    //break;
  case RealDriver::DRIVERTYPE_DX11:
    PSLOG(4) << "Initializing DirectX11 Driver" << std::endl;
    new psDirectX11(dim, init.antialias, init.vsync, init.mode == PSINIT::MODE_FULLSCREEN, init.sRGB, _window);
    if(((psDirectX11*)_driver)->GetLastError() != 0) { delete _driver; _driver = 0; }
    break;
  }
  if(!_driver)
  {
    PSLOGV(0, "Driver encountered a fatal error");
    Quit();
    return;
  }

  STATEBLOCK_LIBRARY::INITLIBRARY();
  psStateblock::DEFAULT = psStateblock::Create(0, 0);
  _driver->SetStateblock(psStateblock::DEFAULT->GetSB());
  
  if(_guiflags&PSGUIMANAGER_LOCKCURSOR) // Ensure the mouse cursor is actually locked
    _dolockcursor(_window);
  _mainpass = new psPass();
  _passes[0] = _mainpass;
}
psEngine::~psEngine()
{
  PROFILE_FUNC();
  if(psRoot::Instance())
    psRoot::Instance()->~psRoot();

  if(_driver)
    delete _driver;
  _driver=0;

  if(_mainpass)
    delete _mainpass;
  _mainpass=0;

  _instance=0;
}
bool psEngine::Begin(unsigned int clearcolor)
{
  PROFILE_FUNC();
  if(GetQuit())
    return false;
  if(!_driver->Begin()) //lost device (DX9 only)
    return false;
  _curpass = 0;
  _driver->Clear(clearcolor);
  _passes[0]->Begin();
  return true;
}
bool psEngine::Begin()
{
  PROFILE_FUNC();
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
  if(_curpass >= _passes.Capacity()) return false;
  _passes[_curpass]->End();
  if(++_curpass >= _passes.Capacity()) return false;
  _passes[_curpass]->Begin();
  return true;
}
psEngine* psEngine::Instance()
{
  return _instance;
}
bool psEngine::InsertPass(psPass& pass, unsigned short index)
{
  if(index>_passes.Capacity())
    return false;
  _passes.Insert(&pass, index);
  return true;
}
bool psEngine::RemovePass(unsigned short index)
{
  if(!index || index>=_passes.Capacity()) return false;
  _passes.Remove(index);
  return true;
}
void psEngine::_onresize(unsigned int width, unsigned int height)
{
  if(_driver && _driver->GetBackBuffer())
    _driver->Resize(psVeciu(width, height), _driver->GetBackBuffer()->GetFormat(), -1);
}
void psEngine::Resize(psVeciu dim, PSINIT::MODE mode)
{
  _mode = mode;
  dim = _resizewindow(dim, _mode);
  _driver->Resize(dim, _driver->GetBackBuffer()->GetFormat(), _mode == PSINIT::MODE_FULLSCREEN);
}
