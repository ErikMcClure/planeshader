// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __STATEBLOCK_H__PS__
#define __STATEBLOCK_H__PS__

#include "ps_dec.h"
#include "bss-util/cHash.h"
#include "bss-util/cArray.h"
#include "bss-util/cRefCounter.h"

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
    TYPE_DEPTH_FRONT_STENCILFAILOP,
    TYPE_DEPTH_FRONT_STENCILDEPTHFAILOP,
    TYPE_DEPTH_FRONT_STENCILPASSOP,
    TYPE_DEPTH_FRONT_STENCILFUNC,
    TYPE_DEPTH_BACK_STENCILFAILOP,
    TYPE_DEPTH_BACK_STENCILDEPTHFAILOP,
    TYPE_DEPTH_BACK_STENCILPASSOP,
    TYPE_DEPTH_BACK_STENCILFUNC,
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
    TYPE_TEXTURE_CONSTANT,
  };
  struct STATEINFO
  {
    union {
      struct {
        union {
          unsigned int value;
          float valuef;
        };
        union {
          struct {
            STATETYPE type;
            unsigned char index;
          };
          unsigned short typeindex;
        };
      };
      unsigned __int64 __vali64;
    };
  };
  class PS_DLLEXPORT psStateblock : public bss_util::cRefCounter
  {
  public:
    inline void* GetSB() const { return _sb; }

    static psStateblock* BSS_FASTCALL Create(unsigned int numstates, ...);
    static psStateblock* BSS_FASTCALL Create(const STATEINFO* infos, unsigned int numstates);
    inline static bool SBLESS(const planeshader::STATEINFO& l, const planeshader::STATEINFO& r) { return l.typeindex<r.typeindex; }
    inline static khint_t SBHASHFUNC(psStateblock* sb) {
      unsigned short sz=sb->_infos.Size();
      khint32_t r=0;
      for(unsigned short i = 0; i < sz; ++i)
        r=bss_util::KH_INT64_HASHFUNC((((__int64)bss_util::KH_INT64_HASHFUNC(sb->_infos[i].__vali64))<<32)|r);
      return r;
    }
    inline static bool SBEQUALITY(psStateblock* left, psStateblock* right)
    {
      unsigned short sl=left->_infos.Size();
      unsigned short sr=right->_infos.Size();
      if(sl!=sr) return false;
      for(unsigned short i = 0; i < sl; ++i)
        if(left->_infos[i].__vali64 != left->_infos[i].__vali64)
          return false;
      return true;
    }

    static const int MAXSAMPLERS = 16;

  protected:
    psStateblock(const STATEINFO* infos, unsigned int numstates);
    ~psStateblock();
    virtual void DestroyThis();

    bss_util::WArray<STATEINFO, unsigned short>::t _infos;
    void* _sb;

    typedef bss_util::cKhash<psStateblock*, char, false, &SBHASHFUNC, &SBEQUALITY> BLOCKHASH;
    static BLOCKHASH _blocks;
  };
}

#endif