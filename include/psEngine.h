// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __ENGINE_H__PS__
#define __ENGINE_H__PS__

#include "bss-util/cArraySimple.h"
#include "bss-util/cBitField.h"
#include "bss-util/cHighPrecisionTimer.h"
#include "bss-util/bss_log.h"
#include "psDriver.h"

#define PSLOG(level) BSSLOG(*psEngine::Instance(),level)

namespace planeshader {
  class psPass;
  class psDriver;
  struct PSINIT;

  // Core engine object
  class PS_DLLEXPORT psEngine : public bss_util::cHighPrecisionTimer, public bss_util::cLog, public psDriverHold
  {
    const unsigned char PSENGINE_QUIT = (1<<0);
    const unsigned char PSENGINE_AUTOANI = (1<<1);

  public:
    // Constructor
    psEngine(const PSINIT& init);
    ~psEngine();
    // Begins a frame. Returns false if rendering should stop.
    bool Begin();
    // Ends a frame
    void End();
    void End(double delta);
    // Renders the next pass
    void NextPass();
    // Begins and ends a frame, returning false if rendering should stop.
    inline bool Render() {
      if(!Begin()) return false;
      End();
      return true;
    }
    // Gets a pass. The 0th pass always exists.
    inline psPass* GetPass(unsigned short index=0) const { return index<_passes.Size()?_passes[index]:0; }
    inline unsigned short NumPass() const { return _passes.Size(); }
    inline void Quit() { _flags+=PSENGINE_QUIT; }
    inline bool GetQuit() const { return _flags[PSENGINE_QUIT]; }
    inline void SetAutoUpdateAni(bool ani) { _flags[PSENGINE_AUTOANI]=ani; }
    inline psDriver* GetDriver() const { return _driver; }

    psPass& operator [](unsigned short index) { assert(index<_passes.Size()); return *_passes[index]; }
    const psPass& operator [](unsigned short index) const { assert(index<_passes.Size()); return *_passes[index]; }

    const double& delta; //delta in milliseconds
    const double& secdelta; //delta in seconds

    static psEngine* Instance(); // Cannot be inline'd for DLL reasons.

  protected:
    bss_util::WArray<psPass*, unsigned short>::t _passes;
    bss_util::cBitField<unsigned char> _flags;
    double _secdelta;
    unsigned short _curpass;

    static psEngine* _instance;
  };

  struct PSINIT
  {
    int width;
    int height;
    RealDriver::DRIVERTYPE driver;

  };
}

#endif