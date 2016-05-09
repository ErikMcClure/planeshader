// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __DXGI_H__PS__
#define __DXGI_H__PS__

#include "psDriver.h"
#include "bss-util\bss_win32_includes.h"
#include "directx\DXGI.h"

namespace planeshader {
  // Common interface to DXGI for DirectX10, DirectX11 and DirectX12
  class psDXGI
  {
  public:
    static const char* shader_profiles[];
    static DXGI_SAMPLE_DESC DEFAULT_SAMPLE_DESC;
    static DXGI_FORMAT BSS_FASTCALL FMTtoDXGI(FORMATS format);
    static FORMATS BSS_FASTCALL DXGItoFMT(DXGI_FORMAT format);
    static const char* BSS_FASTCALL GetDXGIError(HRESULT err);

  protected:
    IDXGIAdapter* _createfactory(HWND hwnd, IDXGIOutput*& out);

    IDXGIFactory* _factory;
    DXGI_SAMPLE_DESC _samples;
  };
}

#endif
