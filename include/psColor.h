// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __COLOR_H__PS__
#define __COLOR_H__PS__

#include "ps_dec.h"
#include "bss-util/bss_util.h"
#include "bss-util/bss_sse.h"
#include "psVec.h"

//#define getA(c) (((c)&0xff000000)>>24)
//#define getR(c) (((c)&0x00ff0000)>>16)
//#define getG(c) (((c)&0x0000ff00)>>8)
//#define getB(c) ((c)&0x000000ff)

namespace planeshader {
  BSS_ALIGNED_STRUCT(16) BSS_COMPILER_DLLEXPORT psColor : bss_util::Vector<float, 4>
  {
    typedef bss_util::Vector<float, 4> BASE;
    using BASE::a;
    using BASE::r;
    using BASE::g;
    using BASE::b;
    using BASE::v;

    explicit inline psColor(unsigned int argb) { operator=(argb); } // operator= is SSE optimized in this case
    explicit inline psColor(const unsigned char(&rgba)[4]) : BASE(rgba[0]/255.0f, rgba[1]/255.0f, rgba[2]/255.0f, rgba[3]/255.0f) {}
    //inline psColor(unsigned char r_, unsigned char g_, unsigned char b_, unsigned char a_=255) : BASE(r_/255.0f,g_/255.0f,b_/255.0f,a_/255.0f) { }
    explicit inline psColor(const float(&rgba)[4]) : BASE(rgba[0],rgba[1],rgba[2],rgba[3]) { }
    explicit inline psColor(const double(&rgba)[4]) : BASE((float)rgba[0],(float)rgba[1],(float)rgba[2],(float)rgba[3]) { }
    inline psColor(float r_, float g_, float b_, float a_=1.0f) : BASE(r_,g_,b_,a_) { }
    inline psColor(double r_, double g_, double b_, double a_=1.0f) : BASE((float)r_,(float)g_,(float)b_,(float)a_) { }
    explicit inline psColor(psVec3D rgb, float alpha=1.0f) : BASE(rgb.x,rgb.y,rgb.z,alpha) { }
    explicit inline psColor(float rgb=0.0f, float alpha=1.0f) : BASE(rgb,rgb,rgb,alpha) { }
    inline const psColor ToHSVA() const
    {
      float l = bssmin(r, bssmin(g, b));
      float u= bssmax(r, bssmax(g, b));
      float d = u - l;

      float h;
      if(r == u) h = (g - b) / d;
      else if(g == u) h = 2 + (b - r) / d;
      else h = 4 + (r - g) / d;
      if(h<0) h+=6;

      return psColor(h, d/u, u, a);
    }

    // Swizzles!
    inline const psColor& rgba() const { return *this; }
    inline psColor argb() const { return psColor(a, r, g, b); }
    inline psColor bgra() const { return psColor(b, g, r, a); }
    inline psColor abgr() const { return psColor(a, b, g, r); }
    inline psVec3D rgb() const { return psVec3D(r, g, b); }
    inline psVec3D arg() const { return psVec3D(a, r, g); }
    inline psVec3D arb() const { return psVec3D(a, r, b); }
    inline psVec3D agb() const { return psVec3D(a, g, b); }
    inline psVec ar() const { return psVec(a, r); }
    inline psVec ag() const { return psVec(a, g); }
    inline psVec ab() const { return psVec(a, b); }
    inline psVec rg() const { return psVec(r, g); }
    inline psVec rb() const { return psVec(r, b); }
    inline psVec gb() const { return psVec(g, b); }

    inline const psColor operator+(const psColor& c) const { psColor ret; sseVec xl(v); sseVec xr(c.v); (xl+xr) >> ret.v; return ret; }
    inline const psColor operator-(const psColor& c) const { psColor ret; sseVec xl(v); sseVec xr(c.v); (xl-xr) >> ret.v; return ret; }
    inline const psColor operator*(const psColor& c) const { psColor ret; sseVec xl(v); sseVec xr(c.v); (xl*xr) >> ret.v; return ret; }
    inline const psColor operator/(const psColor& c) const { psColor ret; sseVec xl(v); sseVec xr(c.v); (xl/xr) >> ret.v; return ret; }
    inline psColor& operator+=(const psColor& c) { sseVec xl(v); sseVec xr(c.v); (xl+xr) >> v; return *this; }
    inline psColor& operator-=(const psColor& c) { sseVec xl(v); sseVec xr(c.v); (xl-xr) >> v; return *this; }
    inline psColor& operator*=(const psColor& c) { sseVec xl(v); sseVec xr(c.v); (xl*xr) >> v; return *this; }
    inline psColor& operator/=(const psColor& c) { sseVec xl(v); sseVec xr(c.v); (xl/xr) >> v; return *this; }
    inline bool operator==(const psColor& c) { return (c.a==a)&&(c.r==r)&&(c.g==g)&&(c.b==b); }
    inline bool operator!=(const psColor& c) { return (c.a!=a)||(c.r!=r)||(c.g!=g)||(c.b!=b); }
    inline psColor& operator=(const psVec3D& rgb) { r=rgb.x; r=rgb.y; r=rgb.z; return *this; }
    inline psColor& operator=(unsigned int argb) {
      sseVec c(_mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(argb), _mm_setzero_si128()), _mm_setzero_si128()));
      sseVec(_mm_castsi128_ps(BSS_SSE_SHUFFLE_EPI32(_mm_castps_si128(c/sseVec(255.0f)), _MM_SHUFFLE(3, 0, 1, 2)))) >> v; return *this;
    }
    inline psColor& operator=(const unsigned char(&rgba)[4]) { r=rgba[0]/255.0f; g=rgba[1]/255.0f; b=rgba[2]/255.0f; a=rgba[3]/255.0f; return *this; }
    inline psColor& operator=(const float(&rgba)[4]) { r=rgba[0]; g=rgba[1]; b=rgba[2]; a=rgba[3]; return *this; }
    inline operator unsigned int() const
    {
      sseVeci xch(sseVec(v)*sseVec(255.0f, 255.0f, 255.0f, 255.0f));
      xch = _mm_packus_epi16(_mm_packs_epi32(BSS_SSE_SHUFFLE_EPI32(xch, _MM_SHUFFLE(3, 0, 1, 2)), _mm_setzero_si128()), _mm_setzero_si128());
      return (unsigned int)_mm_cvtsi128_si32(xch);
    }
    inline operator const float*() const { return v; }

    // Interpolates between two colors
    inline static unsigned int BSS_FASTCALL Interpolate(unsigned int l, unsigned int r, float c)
    {
      sseVec xl(_mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(l), _mm_setzero_si128()), _mm_setzero_si128())); // unpack left integer into floats (WITHOUT normalization)
      sseVec xr(_mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(r), _mm_setzero_si128()), _mm_setzero_si128())); // unpack right integer
      sseVec xc(c); // (c,c,c,c)
      sseVec xx = (xr*xc) + (xl*(sseVec(1.0f)-xc)); // Do operation (r*c) + (l*(1-c))
      sseVeci xch = _mm_packus_epi16(_mm_packs_epi32(sseVeci(xx), _mm_setzero_si128()), _mm_setzero_si128()); // Convert floats back to integers (WITHOUT renormalization) and repack into integer
      return (unsigned int)_mm_cvtsi128_si32(xch);
    }

    // Multiplies a color by a value (equivalent to interpolating between r and 0x00000000)
    inline static unsigned int BSS_FASTCALL Multiply(unsigned int r, float c)
    {
      sseVec xr(_mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(r), _mm_setzero_si128()), _mm_setzero_si128())); // unpack r
      sseVec xc(c); // (c,c,c,c)
      sseVec xx = xr*xc; // Do operation (r*c)
      sseVeci xch = _mm_packus_epi16(_mm_packs_epi32(sseVeci(xx), _mm_setzero_si128()), _mm_setzero_si128()); // Convert floats back to integers (WITHOUT renormalization) and repack into integer
      return (unsigned int)_mm_cvtsi128_si32(xch);
    }

    inline static psColor BSS_FASTCALL Interpolate(const psColor& l, const psColor& r, float c)
    {
      sseVec xc(c);
      psColor ret;
      ((sseVec(l.v)*(sseVec(1.0f)-xc)) + (sseVec(r.v)*xc)) >> ret.v;
      return ret;
    }
    BSS_FORCEINLINE static sseVec BSS_FASTCALL Saturate(const sseVec& x)
    {
      return x.min(sseVec::ZeroVector()).max(sseVec(1.0f));
    }
    inline static psColor BSS_FASTCALL FromHSV(const psVec3D& hsv) { return FromHSVA(psColor(hsv)); }
    inline static psColor BSS_FASTCALL FromHSVA(const psColor& hsva)
    {
      float h=hsva.r;
      float b=hsva.b*(1-hsva.g);
      psColor r;
      (Saturate(sseVec(abs(3.0f-h)-1.0f, 4.0f-h, h-2, 1))*sseVec(1, bssmin(1, h), bssmin(1, 6.0f-h), 1)*sseVec(1-b)*sseVec(hsva.b-b) + sseVec(b)) >> r.v;
      r.a=hsva.a;
      return r;
    }
    // Builds a 32-bit color from 8-bit components
    BSS_FORCEINLINE static unsigned int BSS_FASTCALL BuildColor(unsigned char a, unsigned char r, unsigned char g, unsigned char b)
    {
      unsigned int ret;
      unsigned char* c=(unsigned char*)(&ret);
      c[0]=a;
      c[1]=r;
      c[2]=g;
      c[3]=b;
      return ret;
    }
  };

  // Helper struct that makes accessing the 8-bit channels in 32-bit color easier.
  BSS_ALIGNED_STRUCT(16) BSS_COMPILER_DLLEXPORT psColor32
  {
    psColor32(unsigned char A, unsigned char R, unsigned char G, unsigned char B) : a(A), r(R), g(G), b(B) {}
    explicit psColor32(unsigned char(&c)[4]) : a(c[0]), r(c[1]), g(c[2]), b(c[3]) {}
    psColor32(unsigned __int32 c) : color(c) {}
    psColor32& operator=(unsigned __int32 c) { color=c; return *this; }
    operator unsigned __int32() const { return color; }
    union
    {
      unsigned __int32 color;
      unsigned char colors[4];
      struct {
        unsigned char a;
        unsigned char r;
        unsigned char g;
        unsigned char b;
      };
    };
  };
}

#endif