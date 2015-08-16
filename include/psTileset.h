// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __TILESET_H__PS__
#define __TILESET_H__PS__

#include "psSolid.h"
#include "psDriver.h"
#include "psTextured.h"
#include "bss-util\cDynArray.h"

namespace planeshader {
  struct DEF_TILESET;

  struct psTile
  {
    size_t indice;
    float rotate;
    psVec pivot;
    unsigned int color;
  };

  class PS_DLLEXPORT psTileset : public psSolid, public psTextured, public psDriverHold
  {
  public:
    psTileset(const psTileset& copy);
    psTileset(psTileset&& mov);
    psTileset(const DEF_TILESET& def);
    explicit psTileset(const psVec3D& position = VEC3D_ZERO, FNUM rotation = 0.0f, const psVec& pivot = VEC_ZERO, FLAG_TYPE flags = 0, int zorder = 0, psStateblock* stateblock = 0, psShader* shader = 0, psPass* pass = 0, psInheritable* parent = 0, const psVec& scale = VEC_ONE);

    virtual psTex* const* GetTextures() const { return psTextured::GetTextures(); }
    virtual unsigned char NumTextures() const { return psTextured::NumTextures(); }
    virtual psTex* const* GetRenderTargets() const { return psTextured::GetRenderTargets(); }
    virtual unsigned char NumRT() const { return psTextured::NumRT(); }

  protected:
    virtual void _render();

    size_t _rowlength;
    psVeci _tiledim; // Size of the actual tile for figuring out where to put each tile.
    psVeci _texdim; // Size of the tile texture for UV coordinate generation
    psVeci _dimdiscard;
    psVeci _dimexpand;
    bss_util::cDynArray<psTile> _tiles;
  };


  struct BSS_COMPILER_DLLEXPORT DEF_TILESET : DEF_SOLID, DEF_TEXTURED
  {
    inline DEF_TILESET() {}
    inline virtual psTileset* BSS_FASTCALL Spawn() const { return new psTileset(*this); } //LINKER ERRORS IF THIS DOESNT EXIST! - This should never be called (since the class itself is abstract), but we have to have it here to support inherited classes
    inline virtual DEF_TILESET* BSS_FASTCALL Clone() const { return new DEF_TILESET(*this); }

  };
}
#endif