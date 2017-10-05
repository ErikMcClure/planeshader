// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#ifndef __DXGI_H__PS__
#define __DXGI_H__PS__

#include "psDriver.h"
#include "bss-util/win32_includes.h"
#ifdef USE_DIRECTXTK
#include <dxgi.h>
#else
#include "directx/dxgi.h"
#endif

namespace planeshader {
  // Common interface to DXGI for DirectX10, DirectX11 and DirectX12
  class psDXGI
  {
  public:
    static const char* shader_profiles[];
    static DXGI_SAMPLE_DESC DEFAULT_SAMPLE_DESC;
    static DXGI_FORMAT FMTtoDXGI(FORMATS format);
    static FORMATS DXGItoFMT(DXGI_FORMAT format);
    static const char* GetDXGIError(HRESULT err);

  protected:
    IDXGIAdapter* _createfactory(HWND hwnd, IDXGIOutput*& out);

    IDXGIFactory* _factory;
    DXGI_SAMPLE_DESC _samples;
  };
}

#endif
