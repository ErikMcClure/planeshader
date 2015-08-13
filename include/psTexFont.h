// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __TEXFONT_H__PS__
#define __TEXFONT_H__PS__

#include "psRect.h"
#include "psDriver.h"
#include "bss-util/cRefCounter.h"
#include "bss-util/cHash.h"
#include "bss-util/cArray.h"
#include "bss-util/delegate.h"

namespace planeshader {
  struct psGlyph
  {
    psRect uv; //uv coordinates to glyph on texture
    float ascender; //this will be added to whatever our current line height is.
    float advance; //this will be added to the horizontal position before rendering.
    float width; //this is the font width of the glyph (not how wide the graphic is)
    unsigned char texnum;
  };

  enum TEXT_DRAWFLAGS : unsigned short {
    TDT_RIGHT=(1<<0), //right-aligns the text. If the width is infinite this has no effect.
    TDT_CENTER=(1<<1), //horizontally centers the text.  If the width is infinite this has no effect
    TDT_BOTTOM=(1<<2), //bottom-aligns the text. If the height is infinite this has no effect
    TDT_VCENTER=(1<<3), //vertically centers the text. If the height is infinite this has no effect
    TDT_CLIP=(1<<4), //Clips characters outside of the rectangle.
    TDT_WORDBREAK=(1<<5), //Causes the text to break on words when the end of the rectangle is reached.
    TDT_CHARBREAK=(1<<6), //Causes the text to break on characters when the end of the rectangle is reached.
    TDT_PIXELSNAP=(1<<7), //Snaps all characters to integral coordinates to enhance drawing precision. Not recommended for moving text. Cannot compensate for the camera position.
    TDT_RTL=(1<<8) //Forces right to left rendering
  };

  // Represents any arbitrary font created out of a texture. This is used to implement psFont.
  class PS_DLLEXPORT psTexFont : public bss_util::cRefCounter, protected psDriverHold
  {
  public:
    typedef bss_util::delegate<void, size_t, psRectRotateZ&, unsigned int&> DELEGATE;
    // Draws text in the given rectangle. Returns the dimensions of the text which can be cached used to assemble an area if the text doesn't change.
    psVec DrawText(const int* text, psRect area = RECT_ZERO, unsigned short drawflags = 0, FNUM Z = 0, unsigned int color = 0xFFFFFFFF, FLAG_TYPE flags = 0, psVec textdim = VEC_ZERO, float letterspacing = 0.0f, DELEGATE d = DELEGATE(0,0));
    psVec DrawText(const char* text, psRect area = RECT_ZERO, unsigned short drawflags = 0, FNUM Z = 0, unsigned int color = 0xFFFFFFFF, FLAG_TYPE flags = 0, psVec textdim = VEC_ZERO, float letterspacing = 0.0f, DELEGATE d = DELEGATE(0, 0));
    // Given the drawing flags and text, this calculates what size would be required to display all the text. If dest has nonzero width or height, that dimension is kept constant while calculating the other. If both are nonzero, it is treated as though both were zero in case you forgot to zero-initialize. 
    void CalcTextDim(const int* text, psVec& dest, unsigned short drawflags=0, float letterspacing = 0.0f);
    // Lets you access the underlying textures
    inline const psTex* GetTex(unsigned short index=0) const { assert(index<_textures.Size()); return _textures[index]; }
    // Adds a glyph definition
    void AddGlyph(int character, const psGlyph& glyph);
    // Adds a texture
    unsigned char AddTexture(psTex* tex);
    virtual void DestroyThis();

    static psTexFont* CreateTexFont(psTex* tex=0, float lineheight=0);

  protected:
    psTexFont(const psTexFont& copy);
    psTexFont(psTexFont&& mov);
    psTexFont(psTex* tex, float lineheight);
    explicit psTexFont(float lineheight);
    virtual ~psTexFont();
    float _getlinewidth(const int*& text, float maxwidth, unsigned short drawflags, float letterspacing, float& cur);
    virtual psGlyph* _loadglyph(unsigned int codepoint);
    static bool _isspace(int c);

    bss_util::cHash<int, psGlyph> _glyphs;
    bss_util::cArray<psTex*, unsigned char> _textures;
    float _lineheight;
  };
}

#endif