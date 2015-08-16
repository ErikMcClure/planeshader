// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __TEXTURED_H__PS__
#define __TEXTURED_H__PS__

#include "psTex.h"
#include "bss-util/cArray.h"
#include <vector>

namespace planeshader {
  struct DEF_TEXTURED;

  // Represents any object that can be textured
  class PS_DLLEXPORT psTextured
  {
  public:
    psTextured(const psTextured& copy);
    psTextured(psTextured&& mov);
    psTextured(const DEF_TEXTURED& def);
    explicit psTextured(const char* file);
    explicit psTextured(psTex* tex = 0);
    virtual ~psTextured();

    virtual void BSS_FASTCALL SetTexture(psTex* tex, unsigned int index = 0);
    void BSS_FASTCALL ClearTextures();
    inline const psTex* GetTexture(unsigned int index = 0) const { if(index>=_tex.Size()) return 0; return _tex[index]; }
    virtual inline psTex* const* GetTextures() const { return _tex; }
    virtual inline unsigned char NumTextures() const { return _tex.Size(); }
    virtual inline psTex* const* GetRenderTargets() const { return _rts; }
    virtual inline unsigned char NumRT() const { return _rts.Size(); }
    void BSS_FASTCALL SetRT(psTex* rt, unsigned int index = 0);
    inline virtual psTextured* BSS_FASTCALL Clone() const { return new psTextured(*this); } //Clone function

    psTextured& operator=(const psTextured& right);
    psTextured& operator=(psTextured&& right);

  protected:
    bss_util::cArray<psTex*, unsigned char> _tex;
    bss_util::cArray<psTex*, unsigned char> _rts;
  };

  struct BSS_COMPILER_DLLEXPORT DEF_TEXTURED
  {
    inline virtual psTextured* BSS_FASTCALL Spawn() const { return 0; } //This creates a new instance of whatever class this definition defines
    inline virtual DEF_TEXTURED* BSS_FASTCALL Clone() const { return new DEF_TEXTURED(*this); }

    std::vector<psTex*> texes;
  };
}

#endif