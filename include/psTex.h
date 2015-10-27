// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __TEX_H__PS__
#define __TEX_H__PS__

#include "psDriver.h"
#include "psStateblock.h"
#include "bss-util/cRefCounter.h"
#include "bss-util/cSmartPtr.h"

namespace planeshader {
  // Encapsulates an arbitrary texture not necessarily linked to an actual image
  class PS_DLLEXPORT psTex : public bss_util::cRefCounter, psDriverHold // The reference counter is optional
  {
  public:
    psTex(psTex&& mov);
    psTex(const psTex& copy);
    psTex(psVeciu dim, FORMATS format, unsigned int usage, unsigned char miplevels=0, psTexblock* texblock=0, psVeciu dpi = psVeciu(psDriver::BASE_DPI));
    psTex(void* res, void* view, psVeciu dim, FORMATS format, unsigned int usage, unsigned char miplevels, psTexblock* texblock=0, psVeciu dpi = psVeciu(psDriver::BASE_DPI)); // used to manually set res and view
    ~psTex();
    inline void* GetRes() const { return _res; }
    inline void* GetView() const { return _view; }
    inline psVec GetDim() const { return psVec(_dim)*(psVec((float)psDriver::BASE_DPI) / psVec(_dpi)); }
    inline const psVeciu& GetRawDim() const { return _dim; }
    inline unsigned char GetMipLevels() const { return _miplevels; }
    inline const psTexblock* GetTexblock() const { return _texblock; }
    inline void SetTexblock(psTexblock* texblock) { _texblock = texblock; }
    inline unsigned int GetUsage() const { return _usage; }
    inline FORMATS GetFormat() const { return _format; }
    inline void* Lock(unsigned int& rowpitch, psVeciu offset, unsigned char lockflags = LOCK_WRITE_DISCARD, unsigned char miplevel=0);
    inline void Unlock(unsigned char miplevel=0);
    
    // Attempts to resize the texture using the given method. Returns false if the attempt failed - if the attempt failed, the texture will not have been modified.
    enum RESIZE { RESIZE_DISCARD, RESIZE_CLIP, RESIZE_STRETCH };
    inline bool Resize(psVeciu dim, RESIZE resize = RESIZE_DISCARD);

    // Returns an existing texture object if it has the same path or creates a new one if necessary 
    static psTex* BSS_FASTCALL Create(const char* file, unsigned int usage = USAGE_SHADER_RESOURCE, FILTERS mipfilter = FILTER_TRIANGLE, unsigned char miplevels = 0, FILTERS loadfilter = FILTER_NONE, psVeciu dpi = psVeciu(psDriver::BASE_DPI));
    // if datasize is 0, data is assumed to be a path. If datasize is nonzero, data is assumed to be a pointer to memory where the texture is stored
    static psTex* BSS_FASTCALL Create(const void* data, unsigned int datasize, unsigned int usage = USAGE_SHADER_RESOURCE, FILTERS mipfilter = FILTER_TRIANGLE, unsigned char miplevels = 0, FILTERS loadfilter = FILTER_NONE, psVeciu dpi = psVeciu(psDriver::BASE_DPI));
    static psTex* BSS_FASTCALL Create(const psTex& copy);

    psTex& operator=(const psTex& right);
    psTex& operator=(psTex&& right);

  protected:
    static psTex* BSS_FASTCALL _create(void* res, void* view, psVeciu dpi);

    void BSS_FASTCALL _applydesc(TEXTURE_DESC& desc);
    virtual void DestroyThis();

    void* _res; // In DX10/11 this is the shader resource view. In DX9 it's the texture pointer.
    void* _view; // In DX10/11 this is the render target or depth stencil view. In DX9 it's the surface pointer.
    psVeciu _dim; // Raw dimensions of the texture, not scaled by DPI
    unsigned char _miplevels;
    unsigned int _usage;
    FORMATS _format;
    bss_util::cAutoRef<psTexblock> _texblock;
    psVeciu _dpi; // Actual DPI of this texture. The returned dimensions are scaled by this.
  };
}

#endif