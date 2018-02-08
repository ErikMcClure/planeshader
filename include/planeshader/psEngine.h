// Copyright ©2018 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#ifndef __ENGINE_H__PS__
#define __ENGINE_H__PS__

#include "bss-util/Array.h"
#include "bss-util/BitField.h"
#include "bss-util/Logger.h"
#include "bss-util/Str.h"
#include "bss-util/Serializer.h"
#include "bss-util/Stack.h"
#include "psGUIManager.h"
#include "psDriver.h"
#include "psLayer.h"

#define PSLOG(level,...) psEngine::Instance()->Log(__FILE__,__LINE__,(level),__VA_ARGS__)
#define PSLOGF(level,format,...) psEngine::Instance()->LogFormat(__FILE__,__LINE__,(level),format,__VA_ARGS__)
#define PSLOGP(level,format,...) psEngine::Instance()->PrintLog(__FILE__,__LINE__,(level),format,__VA_ARGS__)

namespace planeshader {
  class psLayer;
  class psDriver;
  class psRoot;

  struct PSINIT
  {
    inline PSINIT() : width(0), height(0), driver(RealDriver::DRIVERTYPE_DX11), mode(psMonitor::MODE_WINDOWED), vsync(false), sRGB(false),
      antialias(0) {}

    int width;
    int height;
    RealDriver::DRIVERTYPE driver;
    psMonitor::MODE mode;
    bool vsync;
    bool sRGB;
    uint8_t antialias;

    template<typename Engine>
    void Serialize(bss::Serializer<Engine>& e, const char*)
    {
      e.EvaluateType<PSINIT>(
        GenPair("width", width),
        GenPair("height", height),
        GenPair("driver", (uint8_t&)driver),
        GenPair("mode", (char&)mode),
        GenPair("vsync", vsync),
        GenPair("sRGB", sRGB),
        GenPair("antialias", antialias)
        );
    }
  };

  // Core engine object
  class PS_DLLEXPORT psEngine : public psGUIManager, public psDriverHold
  {
  public:
    // Constructor
    psEngine(const PSINIT& init, std::ostream* log = 0);
    ~psEngine();
    // Begins a frame. Returns false if rendering should stop.
    bool Begin();
    // Ends a frame, returning the number of nanoseconds between Begin() and End(), before the vsync happened.
    uint64_t End();
    // Renders the next layer, returns false if there are no more top-level layers to render.
    bool NextLayer();
    // Begins and ends a frame, returning false if rendering should stop.
    inline bool Render() {
      if(!Begin()) return false;
      End();
      FlushMessages(); // For best results, calculate the frame delta right before flushing the messages
      return true;
    }
    // Insert a layer 
    bool InsertLayer(psLayer& layer, uint16_t index=-1);
    // Remove layer 
    bool RemoveLayer(uint16_t index);
    // Gets a layer. The 0th layer always exists.
    inline psLayer* GetLayer(uint16_t index=0) const { return index<_layers.Capacity()? _layers[index]:0; }
    inline uint16_t NumLayers() const { return _layers.Capacity(); }
    inline psMonitor::MODE GetMode() const { return _mode; }
    inline bss::Logger& GetLog() { return _log; }

    inline int PrintLog(const char* file, uint32_t line, uint8_t level, const char* format, ...)
    {
      va_list vl;
      va_start(vl, format);
      int r = _log.PrintLogV(LOGSOURCE, file, line, level, format, vl);
      va_end(vl);
      return r;
    }
    inline int PrintLogV(const char* file, uint32_t line, uint8_t level, const char* format, va_list args) { return _log.PrintLog(LOGSOURCE, file, line, level, format, args); }
    template<typename... Args>
    BSS_FORCEINLINE void Log(const char* file, uint32_t line, uint8_t level, Args... args) { _log.Log<Args...>(LOGSOURCE, file, line, level, args...); }
    template<typename... Args>
    BSS_FORCEINLINE void LogFormat(const char* file, uint32_t line, uint8_t level, const char* format, Args... args) { _log.LogFormat<Args...>(LOGSOURCE, file, line, level, format, args...); }

    psLayer& operator [](uint16_t index) { assert(index<_layers.Capacity()); return *_layers[index]; }
    const psLayer& operator [](uint16_t index) const { assert(index<_layers.Capacity()); return *_layers[index]; }

    static psEngine* Instance(); // Cannot be inline'd for DLL reasons.
    static const char* LOGSOURCE;
    static const bssVersionInfo Version;

    bss::LocklessBlockPolicy<psLayer::NODE> NodeAlloc;

  protected:
    virtual void _onresize(psVeciu dim, bool fullscreen) override;

    bss::Array<psLayer*, uint16_t> _layers;
    uint16_t _curlayer;
    psLayer* _mainlayer;
    psMonitor::MODE _mode;
    uint64_t _frameprofiler;
    bss::Logger _log;

    static psEngine* _instance;
  };
}

#endif