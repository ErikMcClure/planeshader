// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __IMAGE_H__PS__
#define __IMAGE_H__PS__

#include "psTextured.h"
#include "psColored.h"
#include "psSolid.h"

namespace planeshader {
  // Represents a renderable image with UV source coordinates
  class PS_DLLEXPORT psImage : public psSolid, public psTextured, public psColored, public psDriverHold
  {
  public:
    psImage(const psImage& copy);
    psImage(psImage&& mov);
    virtual ~psImage();
    explicit psImage(psTex* tex = 0, const psVec3D& position=VEC3D_ZERO, FNUM rotation=0.0f, const psVec& pivot=VEC_ZERO, FLAG_TYPE flags=0, int zorder=0, psStateblock* stateblock=0, psShader* shader=0, psPass* pass = 0, psInheritable* parent=0, const psVec& scale=VEC_ONE, unsigned int color=0xFFFFFFFF);
    explicit psImage(const char* file, const psVec3D& position=VEC3D_ZERO, FNUM rotation=0.0f, const psVec& pivot=VEC_ZERO, FLAG_TYPE flags=0, int zorder=0, psStateblock* stateblock=0, psShader* shader=0, psPass* pass = 0, psInheritable* parent=0, const psVec& scale=VEC_ONE, unsigned int color=0xFFFFFFFF);
    void AddSource(const psRect& r = RECT_UNITRECT);
    void ClearSources();
    inline psRect GetSource(unsigned int index=0) const { if(index>=_uvs.Capacity() || index >= _tex.Capacity()) return RECT_ZERO; return _uvs[index]*_tex[index]->GetDim(); }
    inline const psRect& GetRelativeSource(unsigned int index=0) const { return _uvs[index]; }
    inline const psRect* GetRelativeSources() const { return _uvs; }
    inline void SetRelativeSource(const psRect& uv, unsigned int index = 0) { _setuvs(index+1); _uvs[index] = uv; if(!index) _recalcdim(); }
    inline void SetSource(const psRect& uv, unsigned int index=0) { _setuvs(index+1); _uvs[index] = uv/_tex[index]->GetDim(); if(!index) _recalcdim(); }
    unsigned char NumSources() const { return _uvs.Capacity(); }
    virtual void BSS_FASTCALL SetTexture(psTex* tex, unsigned int index = 0);
    virtual psTex* const* GetTextures() const { return psTextured::GetTextures(); }
    virtual unsigned char NumTextures() const { return psTextured::NumTextures(); }
    void ApplyEdgeBuffer(); // Applies a 1 pixel edge buffer to the image by expanding the UV coordinate out by one pixel at the border to prevent artifacts caused by rasterization.

    psImage& operator =(const psImage& right);
    psImage& operator =(psImage&& right);

  protected:
    virtual void _render();
    virtual void _renderbatch();
    virtual void BSS_FASTCALL _renderbatchlist(psRenderable** rlist, unsigned int count);
    virtual bool BSS_FASTCALL _batch(psRenderable* r) const;
    void _setuvs(unsigned int size);
    void _recalcdim();

    bss_util::cArray<psRect, unsigned char> _uvs;
  };
}

#endif