// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __STATEBLOCK_H__PS__
#define __STATEBLOCK_H__PS__

#include "ps_dec.h"
#include "bss-util/cHash.h"

namespace planeshader {
  enum STATETYPE : unsigned char {
    TYPE_BLEND_ALPHATOCOVERAGEENABLE=0,
    TYPE_BLEND_BLENDENABLE, // index 0-7
    TYPE_BLEND_SRCBLEND,
    TYPE_BLEND_DESTBLEND,
    TYPE_BLEND_BLENDOP,
    TYPE_BLEND_SRCBLENDALPHA,
    TYPE_BLEND_DESTBLENDALPHA,
    TYPE_BLEND_BLENDOPALPHA,
    TYPE_BLEND_RENDERTARGETWRITEMASK, // index 0-7
    TYPE_DEPTH_DEPTHENABLE,
    TYPE_DEPTH_DEPTHWRITEMASK,
    TYPE_DEPTH_DEPTHFUNC,
    TYPE_DEPTH_STENCILENABLE,
    TYPE_DEPTH_STENCILREADMASK,
    TYPE_DEPTH_STENCILWRITEMASK,
    TYPE_DEPTH_FRONTFACE,
    TYPE_DEPTH_BACKFACE,
    TYPE_SAMPLER_FILTER, // all samplerstates have index 0-15 for PS, 16-31 for VS, 32-47 for GS, and 48-63 for CS
    TYPE_SAMPLER_ADDRESSU,
    TYPE_SAMPLER_ADDRESSV,
    TYPE_SAMPLER_ADDRESSW,
    TYPE_SAMPLER_MIPLODBIAS,
    TYPE_SAMPLER_MAXANISOTROPY,
    TYPE_SAMPLER_COMPARISONFUNC,
    TYPE_SAMPLER_BORDERCOLOR1,
    TYPE_SAMPLER_BORDERCOLOR2,
    TYPE_SAMPLER_BORDERCOLOR3,
    TYPE_SAMPLER_BORDERCOLOR4,
    TYPE_SAMPLER_MINLOD,
    TYPE_SAMPLER_MAXLOD,
    TYPE_RASTER_FILLMODE,
    TYPE_RASTER_CULLMODE,
    TYPE_RASTER_FRONTCOUNTERCLOCKWISE,
    TYPE_RASTER_DEPTHBIAS,
    TYPE_RASTER_DEPTHBIASCLAMP,
    TYPE_RASTER_SLOPESCALEDDEPTHBIAS,
    TYPE_RASTER_DEPTHCLIPENABLE,
    TYPE_RASTER_SCISSORENABLE,
    TYPE_RASTER_MULTISAMPLEENABLE,
    TYPE_RASTER_ANTIALIASEDLINEENABLE,
    TYPE_TEXTURE_COLOROP, //legacy support for fixed-function texture blending, index 0-7
    TYPE_TEXTURE_COLORARG1,
    TYPE_TEXTURE_COLORARG2,
    TYPE_TEXTURE_ALPHAOP,
    TYPE_TEXTURE_ALPHAARG1,
    TYPE_TEXTURE_ALPHAARG2,
    TYPE_TEXTURE_TEXCOORDINDEX,
    TYPE_TEXTURE_COLORARG0,
    TYPE_TEXTURE_ALPHAARG0,
    TYPE_TEXTURE_RESULTARG,
    TYPE_TEXTURE_CONSTANT
  };
  struct STATEINFO
  {
    union {
      struct {
        STATETYPE type;
        unsigned char index;
      };
      unsigned short typeindex;
    };
    union {
      unsigned int value;
      float valuef;
    };
  };
  class PS_DLLEXPORT psStateblock
  {
  public:
    static psStateblock* BSS_FASTCALL Create(unsigned int numstates, ...);
    static psStateblock* BSS_FASTCALL Create(const STATEINFO* infos, unsigned int numstates);

  protected:
    psStateblock(const STATEINFO* infos, unsigned int numstates);

    bss_util::cArraySimple<STATEINFO, unsigned short> _infos;
    void* _sb;

    static cHash
  };
}

inline static bool operator<(const planeshader::STATEINFO& l, const planeshader::STATEINFO& r) { return l.typeindex<r.typeindex; }
inline static bool operator>(const planeshader::STATEINFO& l, const planeshader::STATEINFO& r) { return l.typeindex>r.typeindex; }

#endif