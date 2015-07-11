// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __COLORED_H__PS__
#define __COLORED_H__PS__

#include "ps_dec.h"
#include "ps_ani.h"
#include "psColor.h"

namespace planeshader {
  struct DEF_COLORED;

  // Represents something that has color.
  class PS_DLLEXPORT psColored : public AbstractAni
  {
  public:
    psColored(const psColored& copy);
    psColored(const DEF_COLORED& def);
    explicit psColored(unsigned int color=0xFFFFFFFF);
    inline const psColor32& GetColor() const { return _color; }
    virtual void BSS_FASTCALL SetColor(unsigned int color);
    inline void BSS_FASTCALL SetColor(unsigned char a, unsigned char r, unsigned char g, unsigned char b) { SetColor(psColor32(a,r,g,b)); }

    inline psColored& operator=(const psColored& right) { _color = right._color; return *this; }
    inline virtual psColored* BSS_FASTCALL Clone() const { return new psColored(*this); } // Clone function

    // Interpolation functions for animation
    template<unsigned char TypeID>
    static inline unsigned int BSS_FASTCALL colorinterpolate(const typename bss_util::AniAttributeT<TypeID>::TVT_ARRAY_T& rarr, bss_util::AniAttribute::IDTYPE index, double factor) {
      return psColor::Interpolate(rarr[index-1].value, rarr[index].value, factor);
    }

  protected:
    void BSS_FASTCALL _setcolor(unsigned int color) { SetColor(color); }
    virtual void BSS_FASTCALL TypeIDRegFunc(bss_util::AniAttribute*);

    psColor32 _color;
  };

  struct BSS_COMPILER_DLLEXPORT DEF_COLORED
  {
    inline DEF_COLORED() : color(-1) {}
    inline virtual psColored* BSS_FASTCALL Spawn() const { return new psColored(*this); } //This creates a new instance of whatever class this definition defines
    inline virtual DEF_COLORED* BSS_FASTCALL Clone() const { return new DEF_COLORED(*this); }

    DEF_COLORED& operator =(const DEF_COLORED& copy) { color=copy.color; return *this; }
    unsigned int color;
  };
}

#endif