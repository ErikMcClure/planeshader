// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __ENGINE_H__PS__
#define __ENGINE_H__PS__

#include "bss-util/cArray.h"
#include "bss-util/cBitField.h"
#include "bss-util/cHighPrecisionTimer.h"
#include "bss-util/bss_log.h"
#include "bss-util/cStr.h"
#include "psGUIManager.h"
#include "psDriver.h"

#define PSLOG(level) BSSLOG(*psEngine::Instance(),level)
#define PSLOGV(level,...) BSSLOGV(*psEngine::Instance(),level,__VA_ARGS__)

namespace planeshader {
  class psPass;
  class psDriver;

  struct PSINIT
  {
    inline PSINIT() : width(0), height(0), driver(RealDriver::DRIVERTYPE_DX11), mode(MODE_WINDOWED), vsync(false), sRGB(false),
      antialias(0), extent(1.0f, 50000.0f), errout(0), mediapath("") {}

    int width;
    int height;
    RealDriver::DRIVERTYPE driver;
    enum MODE : char {
      MODE_WINDOWED = 0,
      MODE_FULLSCREEN,
      MODE_BORDERLESS_TOPMOST,
      MODE_BORDERLESS,
      MODE_COMPOSITE,
      MODE_COMPOSITE_CLICKTHROUGH,
      MODE_COMPOSITE_NOMOVE,
      MODE_COMPOSITE_OPAQUE_CLICK,
    } mode;
    bool vsync;
    bool sRGB;
    unsigned char antialias;
    psVec extent;
    std::ostream* errout;
    const char* mediapath;
  };

  // Core engine object
  class PS_DLLEXPORT psEngine : public psGUIManager, public bss_util::cHighPrecisionTimer, public bss_util::cLog, public psDriverHold
  {
    static const unsigned char PSENGINE_QUIT = (1<<0);

  public:
    // Constructor
    psEngine(const PSINIT& init);
    ~psEngine();
    // Begins a frame. Returns false if rendering should stop.
    bool Begin();
    bool Begin(unsigned int clearcolor);
    // Ends a frame
    void End();
    void End(double delta);
    // Renders the next pass, returns false if there are no more passes to render.
    bool NextPass();
    // Begins and ends a frame, returning false if rendering should stop.
    inline bool Render() {
      if(!Begin()) return false;
      End();
      return true;
    }
    // Insert a pass 
    bool InsertPass(psPass& pass, unsigned short index=-1);
    // Remove pass 
    bool RemovePass(unsigned short index);
    // Gets a pass. The 0th pass always exists.
    inline psPass* GetPass(unsigned short index=0) const { return index<_passes.Capacity()?_passes[index]:0; }
    inline unsigned short NumPass() const { return _passes.Capacity(); }
    // Get/Sets the quit value
    inline void Quit() { _flags+=PSENGINE_QUIT; }
    inline bool GetQuit() const { return _flags[PSENGINE_QUIT]; }
    // Get/set the timewarp factor
    inline void SetTimeWarp(double timewarp) { _timewarp = timewarp; }
    inline double GetTimeWarp() const { return _timewarp; }
    inline const char* GetMediaPath() const { return _mediapath.c_str(); }
    inline PSINIT::MODE GetMode() const { return _mode; }
    void Resize(psVeciu dim, PSINIT::MODE mode);

    psPass& operator [](unsigned short index) { assert(index<_passes.Capacity()); return *_passes[index]; }
    const psPass& operator [](unsigned short index) const { assert(index<_passes.Capacity()); return *_passes[index]; }

    const double& delta; //delta in milliseconds
    const double& secdelta; //delta in seconds

    static psEngine* Instance(); // Cannot be inline'd for DLL reasons.

  protected:
    virtual void _onresize(unsigned int width, unsigned int height);

    bss_util::cArray<psPass*, unsigned short> _passes;
    bss_util::cBitField<unsigned char> _flags;
    double _secdelta;
    double _timewarp;
    unsigned short _curpass;
    psPass* _mainpass;
    cStr _mediapath;
    PSINIT::MODE _mode;

    static psEngine* _instance;
  };
}

#endif