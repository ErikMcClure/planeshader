// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __TEX_H__PS__
#define __TEX_H__PS__

#include "psDriver.h"
#include "bss-util/cRefCounter.h"

namespace planeshader {
  // Encapsulates an arbitrary texture not necessarily linked to an actual image
  class PS_DLLEXPORT psTex : bss_util::cRefCounter, psDriverHold // The reference counter is completely optional and is usually only used for psTexture
  {
  public:
    psTex(psTex&& mov);
    psTex(const psTex& copy);
    psTex(psVeciu dim, FORMATS format, USAGETYPES usage, unsigned char miplevels=0);
    psTex(void* res, void* view, psVeciu dim, FORMATS format, USAGETYPES usage, unsigned char miplevels); // used to manually set res and view
    ~psTex();
    inline void* GetRes() const { return _res; }
    inline void* GetView() const { return _view; }
    inline const psVeciu& GetDim() const { return _dim; }
    inline unsigned char GetMipLevels() const { return _miplevels; }

    psTex& operator=(const psTex& right);
    psTex& operator=(psTex&& right);

  protected:
    virtual void DestroyThis();

    void* _res; // In DX10/11 this is the shader resource view. In DX9 it's the texture pointer.
    void* _view; // In DX10/11 this is the render target or depth stencil view. In DX9 it's the surface pointer.
    psVeciu _dim;
    unsigned char _miplevels;
    USAGETYPES _usage;
    FORMATS _format;
  };
}

#endif