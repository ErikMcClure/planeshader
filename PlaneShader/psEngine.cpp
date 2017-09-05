// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#include "psEngine.h"
#include "psLayer.h"
#include "psTex.h"
#include "psDirectX11.h"
#include "psVulkan.h"
#include "psNullDriver.h"
#include "psStateblock.h"
#include "bss-util/profiler.h"
#include "ps_feather.h"

using namespace planeshader;
using namespace bss;

#ifdef BSS_COMPILER_MSC
#if defined(BSS_DEBUG) && defined(BSS_CPU_x86_64)
#pragma comment(lib, "bss-util_d.lib")
#elif defined(BSS_CPU_x86_64)
#pragma comment(lib, "bss-util.lib")
#elif defined(BSS_DEBUG)
#pragma comment(lib, "bss-util32_d.lib")
#else
#pragma comment(lib, "bss-util32.lib")
#endif
#endif

psDriver* psDriverHold::_driver=0;
psEngine* psEngine::_instance=0;
const char* psEngine::LOGSOURCE = "ps";
const psMatrix psDriver::identity ={ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
const bssVersionInfo psEngine::Version = { 0, PS_VERSION_REVISION, PS_VERSION_MINOR, PS_VERSION_MAJOR };

psDriver* psDriverHold::GetDriver() { return _driver; }

psEngine::psEngine(const PSINIT& init, std::ostream* log) : _log(!log?"PlaneShader.log":0, log), _curlayer(0), _layers(1),
  _mainlayer(0),  _frameprofiler(0)
{
  PROFILE_FUNC();
  if(_instance!=0) // If you try to make another instance, it violently explodes.
    terminate();
  _instance = this;
  fSetDenormal(false);
  fSetRounding(true);

  _log.GetStream() << "--- Initializing PlaneShader v." << Version.Major << '.' << Version.Minor << '.' << Version.Revision << " ---" << std::endl;

  psVeciu dim(init.width, init.height);
  AddMonitor(dim, init.mode, 0);

  switch(init.driver)
  {
  case RealDriver::DRIVERTYPE_NULL:
    PSLOG(-1, "Initializing Null Driver");
    new psNullDriver();
    break;
  case RealDriver::DRIVERTYPE_VULKAN:
    PSLOG(-1, "Initializing Vulkan Driver");
    new psVulkan(dim, init.antialias, init.vsync, init.mode == psMonitor::MODE_FULLSCREEN, init.sRGB, &_monitors[0]);
    //if(((psVulkan*)_driver)->GetLastError() != 0) { delete _driver; _driver = 0; }
    break;
  case RealDriver::DRIVERTYPE_DX11:
    PSLOG(-1, "Initializing DirectX11 Driver");
    new psDirectX11(dim, init.antialias, init.vsync, init.mode == psMonitor::MODE_FULLSCREEN, init.sRGB, &_monitors[0]);
    if(_driver->GetRealDriver().dx11->GetLastError() != 0) { delete _driver; _driver = 0; }
    break;
  }
  if(!_driver)
  {
    PSLOG(0, "Driver encountered a fatal error");
    Quit();
    return;
  }

  STATEBLOCK_LIBRARY::INITLIBRARY();
  psStateblock::DEFAULT = psStateblock::Create(0, 0);
  _driver->SetStateblock(psStateblock::DEFAULT->GetSB());
  
  //if(_guiflags&PSMONITOR_LOCKCURSOR) // Ensure the mouse cursor is actually locked
  //  _dolockcursor(_window);

  _mainlayer = (psLayer*)ALIGNEDALLOC(sizeof(psLayer), alignof(psLayer));
  new (_mainlayer) psLayer(_monitors[0].GetBackBuffer()); // Attempting to override new in the class itself breaks placement new elsewhere
  _mainlayer->SetCamera(&psCamera::default_camera);
  _layers[0] = _mainlayer;
}
psEngine::~psEngine()
{
  PROFILE_FUNC();

  if(_mainlayer)
  {
    _mainlayer->~psLayer();
    ALIGNEDFREE(_mainlayer);
  }
  _mainlayer = 0;

  _monitors.Clear(); // We must clear and destroy all monitors BEFORE we destroy the fgroot instance

  if(_driver)
    delete _driver;
  _driver=0;

  fgRoot_Destroy(&_root);
  _instance = 0;
}
bool psEngine::Begin()
{
  PROFILE_FUNC();
  if(GetQuit())
    return false;
  if(!_driver->Begin()) //lost device (DX9 only)
    return false;
  _frameprofiler = HighPrecisionTimer::OpenProfiler();
  _curlayer=0;
  _layers[0]->Push(psTransform2D::Zero);
  return true;
}
uint64_t psEngine::End()
{
  PROFILE_FUNC();
  while(NextLayer());

  //if(_realdriver.dx9->_mousehittest!=0) //if this is nonzero we need to toggle transparency based on a system wide mouse hit test
  //  if(_realdriver.dx9->MouseHitTest(GetMouseExact(), _alphacutoff))
  //    SetWindowLong(_window, GWL_EXSTYLE, ((GetWindowLong(_window, GWL_EXSTYLE))&(~WS_EX_TRANSPARENT)));
  //  else
  //    SetWindowLong(_window, GWL_EXSTYLE, ((GetWindowLong(_window, GWL_EXSTYLE))|WS_EX_TRANSPARENT));
  _frameprofiler = HighPrecisionTimer::CloseProfiler(_frameprofiler);
  _driver->End();
  return _frameprofiler;
}
bool psEngine::NextLayer()
{
  PROFILE_FUNC();
  if(_curlayer >= _layers.Capacity()) return false;
  _layers[_curlayer]->Pop();
  if(++_curlayer >= _layers.Capacity()) return false;
  _layers[_curlayer]->Push(psTransform2D::Zero);
  return true;
}
psEngine* psEngine::Instance()
{
  return _instance;
}
bool psEngine::InsertLayer(psLayer& layer, uint16_t index)
{
  if(index == (uint16_t)~0)
    index = _layers.Capacity();
  if(index > _layers.Capacity())
    return false;
  _layers.Insert(&layer, index);
  return true;
}
bool psEngine::RemoveLayer(uint16_t index)
{
  if(!index || index>= _layers.Capacity()) return false;
  _layers.Remove(index);
  return true;
}
void psEngine::_onresize(psVeciu dim, bool fullscreen)
{
  if(_driver != 0 && _driver->GetBackBuffer() != 0)
    _driver->Resize(dim, _driver->GetBackBuffer()->GetFormat(), fullscreen);
}
