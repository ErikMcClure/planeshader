// Copyright (c)2026 Erik McClure
// For conditions of distribution and use, see copyright notice in ps_dec.h

#ifndef __TEXTURED_H__PS__
#define __TEXTURED_H__PS__

#include "psTex.h"
#include "buntils/CompactArray.h"
#include <vector>

namespace planeshader {
  // Represents any object that can be textured
  class PS_DLLEXPORT psTextured
  {
  public:
    psTextured(const psTextured& copy);
    psTextured(psTextured&& mov);
    explicit psTextured(const char* file);
    explicit psTextured(psTex* tex = 0);
    ~psTextured();

    virtual void SetTexture(psTex* tex, size_t index = 0);
    void ClearTextures();
    inline psTex* GetTexture(size_t index = 0) const { if(index>=_tex.size()) return 0; return _tex[index]; }
    inline psTex* const* GetTextures() const { return _tex; }
    inline size_t NumTextures() const { return _tex.size(); }

    psTextured& operator=(const psTextured& right);
    psTextured& operator=(psTextured&& right);

  protected:
    bun::CompactArray<psTex*> _tex;
  };
}

#endif