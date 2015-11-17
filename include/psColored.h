// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __COLORED_H__PS__
#define __COLORED_H__PS__

#include "ps_dec.h"
#include "psColor.h"

namespace planeshader {
  // Represents something that has color.
  class PS_DLLEXPORT psColored
  {
  public:
    psColored(const psColored& copy);
    explicit psColored(unsigned int color=0xFFFFFFFF);
    virtual ~psColored();
    inline const psColor32& GetColor() const { return _color; }
    virtual void BSS_FASTCALL SetColor(unsigned int color);
    inline void BSS_FASTCALL SetColor(unsigned char a, unsigned char r, unsigned char g, unsigned char b) { SetColor(psColor32(a,r,g,b)); }

    inline psColored& operator=(const psColored& right) { _color = right._color; return *this; }
    inline virtual psColored* BSS_FASTCALL Clone() const { return new psColored(*this); } // Clone function

    // Interpolation functions for animation
    //template<unsigned char TypeID>
    //static inline unsigned int BSS_FASTCALL colorinterpolate(const typename bss_util::AniAttributeT<TypeID>::TVT_ARRAY_T& rarr, bss_util::AniAttribute::IDTYPE index, double factor) {
    //  return psColor::Interpolate(rarr[index-1].value, rarr[index].value, factor);
    //}

  protected:
    void BSS_FASTCALL _setcolor(unsigned int color) { SetColor(color); }

    psColor32 _color;
  };
}


#endif