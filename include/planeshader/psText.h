// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#ifndef __TEXT_H__PS__
#define __TEXT_H__PS__

#include "psTexFont.h"
#include "psSolid.h"
#include "psColor.h"
#include "bss-util/Str.h"
#include "bss-util/ArraySort.h"

namespace planeshader {
  class PS_DLLEXPORT psText : public psSolid
  {
  public:
    psText(const psText& copy);
    psText(psText&& mov);
    explicit psText(psTexFont* font=0, const char* text = 0, float lineheight = 0.0f, const psVec3D& position = VEC3D_ZERO, FNUM rotation = 0.0f, const psVec& pivot = VEC_ZERO, psFlag flags = 0, int zorder = 0, psStateblock* stateblock = 0, psShader* shader = 0, psLayer* pass = 0, const psVec& scale = VEC_ONE);
    ~psText();
    // Get/Set the text to be rendered
    inline const int* GetText() const { return _text; }
    inline void SetText(const char* text) { _text = text; _recalcdim(); }
    // Get/Set font used
    inline const psTexFont* GetFont() const { return _font; }
    void SetFont(psTexFont* font);
    // Get/Set the dimensions of the textbox - a dimension of 0 will grow to contain the text.
    inline psVec GetSize() const { return _textdim; }
    inline void SetSize(psVec size) { _textdim = size; _recalcdim(); }
    // Get/Set letter spacing
    inline float GetLetterSpacing() const { return _letterspacing; }
    inline void SetLetterSpacing(float spacing) { _letterspacing = spacing; _recalcdim(); }
    // Get/Set draw flags
    inline uint16_t GetDrawFlags() const { return _drawflags; }
    inline void SetDrawFlags(uint16_t flags) { _drawflags = flags; }
    // Get/Set per-character modification function
    inline const psTexFont::DELEGATE& GetFunc() const { return _func; }
    inline void SetFunc(psTexFont::DELEGATE func) { _func = func; }
    // Get/set text color
    inline psColor32 GetColor() const { return _color; }
    inline void SetColor(psColor32 color) { _color = color; }

    psText& operator=(const psText& copy);
    psText& operator=(psText&& mov);

  protected:
    virtual void _render(const psTransform2D& parent) override;
    void _recalcdim();

    bss::StrT<int> _text;
    bss::ref_ptr<psTexFont> _font;
    psVec _textdim;
    float _letterspacing;
    uint16_t _drawflags;
    psTexFont::DELEGATE _func;
    float _lineheight;
    psColor32 _color;
  };
}
#endif