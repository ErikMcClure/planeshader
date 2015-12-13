// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __FEATHER_H__PS__
#define __FEATHER_H__PS__

#include "feathergui\fgRoot.h"
#include "psRenderable.h"

namespace planeshader {
  class PS_DLLEXPORT psRoot : public psDriverHold, public psRenderable
  {
  public:
    psRoot();
    ~psRoot();
    fgRoot& GetRoot() { return _root; }
    bool BSS_FASTCALL ProcessGUI(const psGUIEvent& evt);

    static psRoot* Instance();

    static FLAG_TYPE GetDrawFlags(fgFlag flags);

  protected:
    void _render();

    static psRoot* instance;

    fgRoot _root;
    bss_util::delegate<bool, const psGUIEvent&> _prev;
  };
}

#endif