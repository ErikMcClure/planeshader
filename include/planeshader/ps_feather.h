// Copyright ©2018 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#ifndef __FEATHER_H__PS__
#define __FEATHER_H__PS__

#include "psMonitor.h"
#include "psRenderable.h"
#include "bss-util/Delegate.h"

namespace planeshader {
  class PS_DLLEXPORT psRoot : public fgRoot, public psDriverHold, public psRenderable
  {
  public:
    typedef bss::Delegate<size_t, const FG_Msg&> PS_MESSAGE;

    psRoot();
    ~psRoot();
    inline PS_MESSAGE GetInject() const { return _psInject; }
    inline void SetInject(PS_MESSAGE fn) { _psInject = fn; fgSetInjectFunc(_psInject.IsEmpty() ? fgRoot_DefaultInject : InjectDelegate); }
    
    static psFlag GetDrawFlags(fgFlag flags);
    static size_t InjectDelegate(fgRoot* self, const FG_Msg* m) { return reinterpret_cast<psRoot*>(self)->_psInject(*m); }

  protected:
    virtual void _render(const psTransform2D& parent) override;

    PS_MESSAGE _psInject;
  };
}

#endif