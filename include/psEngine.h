// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __ENGINE_H__PS__
#define __ENGINE_H__PS__

#include "bss-util/cArray.h"
#include "bss-util/cBitField.h"
#include "bss-util/cHighPrecisionTimer.h"
#include "bss-util/bss_log.h"
#include "psGUIManager.h"
#include "psDriver.h"

#define PSLOG(level) BSSLOG(*psEngine::Instance(),level)
#define PSLOGV(level,...) BSSLOGV(*psEngine::Instance(),level,__VA_ARGS__)

namespace planeshader {
  class psPass;
  class psDriver;
  struct PSINIT;

  // Core engine object
  class PS_DLLEXPORT psEngine : public psGUIManager, public bss_util::cHighPrecisionTimer, public bss_util::cLog, public psDriverHold
  {
    static const unsigned char PSENGINE_QUIT = (1<<0);
    static const unsigned char PSENGINE_AUTOANI = (1<<1);

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
    inline psPass* GetPass(unsigned short index=0) const { return index<_passes.Size()?_passes[index]:0; }
    inline unsigned short NumPass() const { return _passes.Size(); }
    // Get/Sets the quit value
    inline void Quit() { _flags+=PSENGINE_QUIT; }
    inline bool GetQuit() const { return _flags[PSENGINE_QUIT]; }
    // Sets whether the animations should auto-update
    inline void SetAutoUpdateAni(bool ani) { _flags[PSENGINE_AUTOANI]=ani; }
    // Get/set the timewarp factor
    inline void SetTimeWarp(double timewarp) { _timewarp = timewarp; }
    inline double GetTimeWarp() const { return _timewarp; }

    psPass& operator [](unsigned short index) { assert(index<_passes.Size()); return *_passes[index]; }
    const psPass& operator [](unsigned short index) const { assert(index<_passes.Size()); return *_passes[index]; }

    const double& delta; //delta in milliseconds
    const double& secdelta; //delta in seconds

    static psEngine* Instance(); // Cannot be inline'd for DLL reasons.

  protected:
    bss_util::cArray<psPass*, unsigned short> _passes;
    bss_util::cBitField<unsigned char> _flags;
    double _secdelta;
    double _timewarp;
    unsigned short _curpass;
    psPass* _mainpass;

    static psEngine* _instance;
  };

  struct PSINIT
  {
    inline PSINIT() : width(0), height(0), driver(RealDriver::DRIVERTYPE_DX9), fullscreen(false), vsync(false),
      destalpha(true), composite(PS_COMP_NONE), antialias(0), farextent(50000.0f), nearextent(1.0f), errout(0) {}

    int width;
    int height;
    RealDriver::DRIVERTYPE driver;
    bool fullscreen;
    bool vsync;
    bool destalpha;
    enum PS_COMP : char {
      PS_COMP_NONE=0,
      PS_COMP_NOFRAME=1,
      PS_COMP_NOFRAME_CLICKTHROUGH=2,
      PS_COMP_NOFRAME_NOMOVE=3,
      PS_COMP_NOFRAME_OPAQUE_CLICK=4,
      PS_COMP_FRAME=-1,
      PS_COMP_FRAME_TITLE=-2,
      PS_COMP_FRAME_TITLE_CLOSEBUTTON=-3,
      PS_COMP_FRAME_SIZEABLE=-4
    } composite;
    unsigned char antialias;
    float farextent;
    float nearextent;
    std::ostream* errout;
  };
}

#endif