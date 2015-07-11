// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __ABSTRACT_H__PS__
#define __ABSTRACT_H__PS__

#include "bss-util/cAnimation.h"
#include "bss-util/LLBase.h"

namespace planeshader {
  class psTex;

  struct psTexturedAbstract
  {
    virtual psTex* const* GetTextures() const=0;
    virtual unsigned char NumTextures() const=0;
    virtual psTex* const* GetRenderTargets() const=0;
    virtual unsigned char NumRT() const=0;
  };
}
#endif