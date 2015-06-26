// Copyright ©2015 Black Sphere Studios
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
    TYPE_BLEND_BLENDFACTOR, // index 0-3
    TYPE_BLEND_SAMPLEMASK,
    TYPE_DEPTH_DEPTHENABLE,
    TYPE_DEPTH_DEPTHWRITEMASK,
    TYPE_DEPTH_DEPTHFUNC,
    TYPE_DEPTH_STENCILVALUE, // StencilRef value everything is compared to
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
        STATETYPE type;
        unsigned short index;
      };
      unsigned __int64 __vali64;
    };

    typedef bss_util::cArray<STATEINFO, unsigned short> STATEINFOS;

    inline static bool SILESS(const planeshader::STATEINFO& l, const planeshader::STATEINFO& r) { return l.type<=r.type && (l.type<r.type || l.index<r.index); }
    inline static khint_t SIHASHFUNC(STATEINFOS* sb) {
      unsigned short sz=sb->Size();
      khint32_t r=0;
      for(unsigned short i = 0; i < sz; ++i)
        r=bss_util::KH_INT64_HASHFUNC((((__int64)bss_util::KH_INT64_HASHFUNC((*sb)[i].__vali64))<<32)|r);
      return r;
    }
    inline static bool SIEQUALITY(STATEINFOS* left, STATEINFOS* right)
    {
      unsigned short sl=left->Size();
      unsigned short sr=right->Size();
      if(sl!=sr) return false;
      for(unsigned short i = 0; i < sl; ++i)
        if((*left)[i].__vali64 != (*right)[i].__vali64)
          return false;
      return true;
    }
    static STATEINFOS* Exists(STATEINFOS* compare);
    typedef bss_util::cKhash<STATEINFOS*, char, false, &SIHASHFUNC, &SIEQUALITY> BLOCKHASH;
    static BLOCKHASH _blocks;
  };

  class PS_DLLEXPORT psStateblock : public bss_util::cArray<STATEINFO, unsigned short>, public bss_util::cRefCounter
  {
  public:
    inline void* GetSB() const { return _sb; }

    static psStateblock* BSS_FASTCALL Create(unsigned int numstates, ...);
    static psStateblock* BSS_FASTCALL Create(const STATEINFO* infos, unsigned int numstates);

    static psStateblock* DEFAULT;

  protected:
    psStateblock(const STATEINFO* infos, unsigned int numstates);
    ~psStateblock();
    virtual void DestroyThis();
    
    void* _sb;
  };

  // Restricted to sampler infos
  class PS_DLLEXPORT psTexblock : public bss_util::cArray<STATEINFO, unsigned short>, public bss_util::cRefCounter
  {
  public:
    inline void* GetSB() const { return _tb; }
    static psTexblock* BSS_FASTCALL Create(unsigned int numstates, ...);
    static psTexblock* BSS_FASTCALL Create(const STATEINFO* infos, unsigned int numstates);

    static psTexblock* DEFAULT;

  protected:
    psTexblock(const STATEINFO* infos, unsigned int numstates);
    ~psTexblock();
    virtual void DestroyThis();

    void* _tb; // In DX10, this is the sampler state, but in openGL, it's actually NULL, because openGL binds the sampler states to the texture at texture creation time using the STATEINFOs contained in this object.
  };

  struct PS_DLLEXPORT STATEBLOCK_LIBRARY
  {
    static void INITLIBRARY();
    static psStateblock* GLOW; //Basic glow
    static psStateblock* PARTICLE; //turns off alpha test for particles
    static psStateblock* PARTICLE_GLOW; //Both GLOW and PARTICLE
    static psStateblock* MASK[8]; //Used for masking
    static psStateblock* INVMASK[8];
    static psTexblock* UVBORDER; // Sets UV coordinates to use a border of color 0 (you can easily override that)
    static psTexblock* UVMIRROR; // Sets UV coordinates to Mirror
    static psTexblock* UVMIRRORONCE; // Sets UV coordinates to MirrorOnce
    static psTexblock* UVWRAP; // Sets UV coordinates to Wrap (There is no CLAMP as that is the default)
    static psStateblock* GAMMAREAD; // linearize on read (only the 0th sampler, which is usually the diffuse map, since all other maps are already linear) 
    static psStateblock* GAMMAWRITE; // un-linearize on write
    static psStateblock* GAMMACORRECT; // Standard gamma correction
    static psStateblock* SUBTRACTIVE; // Subtracts the image from whatever its being rendered on top of. Used for black LCD text.
    static psStateblock* PREMULTIPLIED; // Blending state for premultiplied textures
  };
}

#endif