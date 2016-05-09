// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __FEATHER_H__PS__
#define __FEATHER_H__PS__

#include "feathergui\fgRoot.h"
#include "feathergui\fgMonitor.h"
#include "psRenderable.h"

struct HWND__;

namespace planeshader {
  class PS_DLLEXPORT psRoot : public fgRoot, public psDriverHold, public psRenderable
  {
  public:
    psRoot();
    ~psRoot();
    
    static psFlag GetDrawFlags(fgFlag flags);

  protected:
    void BSS_FASTCALL _render();
  };

  class PS_DLLEXPORT psMonitor : public fgMonitor
  {
  public:
    // Gets the window handle
    HWND__* GetWindow() const { return _window; }
    // Locks the cursor
    void LockCursor(bool lock);
    // Shows/hides the hardware cursor
    void ShowCursor(bool show);
    // Set window title
    void SetWindowTitle(const char* caption);

    enum MODE : char {
      MODE_WINDOWED = 0,
      MODE_FULLSCREEN,
      MODE_BORDERLESS_TOPMOST,
      MODE_BORDERLESS,
      MODE_COMPOSITE,
      MODE_COMPOSITE_CLICKTHROUGH,
      MODE_COMPOSITE_NOMOVE,
      MODE_COMPOSITE_OPAQUE_CLICK,
    };

    inline MODE GetMode() const { return _mode; }
    void Resize(psVeciu dim, MODE mode);

  protected:
    HWND__* _window;
    MODE _mode;
  };
}

#endif