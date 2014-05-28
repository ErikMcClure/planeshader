// Copyright �2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __PASS_H__PS__
#define __PASS_H__PS__

#include "psDriver.h"
#include "psRenderable.h"

namespace planeshader {
  class PS_DLLEXPORT psPass : public psDriverHold
  {
  public:
    psPass();
    ~psPass();
    void Begin();
    void End();
    inline void BSS_FASTCALL SetCamera(const psCamera* camera) { _cam=!camera?&psCamera::default_camera:camera; }
    inline const psCamera* GetCamera() const { return _cam; }
    void Insert(psRenderable* r);
    void Remove(psRenderable* r);

    static BSS_FORCEINLINE bss_util::LLBase<psRenderable>& GetRenderableAlt(psRenderable* r) { return r->_llist; }

  protected:
    const psCamera* _cam;
    psRenderable* _renderables;
  };
}

#endif