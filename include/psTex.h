// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __TEX_H__PS__
#define __TEX_H__PS__

#include "psVec.h"

namespace planeshader {
  class PS_DLLEXPORT psTex
  {
  public:
    psTex();
    ~psTex();
    inline void* GetRes() const { return _res; }
    inline void* GetView() const { return _view; }

  protected:
    void* _res; // In DX10/11 this is the shader resource view. In DX9 it's the texture pointer.
    void* _view; // In DX10/11 this is the render target or depth stencil view. In DX9 it's the surface pointer.
    psVeciu _dim;
    unsigned int _usage;
  };
}

#endif