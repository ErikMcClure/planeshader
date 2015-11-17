// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#ifndef __TILESET_H__PS__
#define __TILESET_H__PS__

#include "psSolid.h"
#include "psDriver.h"
#include "psTextured.h"
#include "bss-util\cDynArray.h"

namespace planeshader {
  struct psTile
  {
    size_t index;
    float rotate;
    psVec pivot;
    unsigned int color;
  };

  class PS_DLLEXPORT psTileset : public psSolid, public psTextured, public psDriverHold
  {
    struct psTileDef {
      psRect uv;
      psRect rect;
    };

  public:
    psTileset(const psTileset& copy);
    psTileset(psTileset&& mov);
    explicit psTileset(const psVec3D& position = VEC3D_ZERO, FNUM rotation = 0.0f, const psVec& pivot = VEC_ZERO, FLAG_TYPE flags = 0, int zorder = 0, psStateblock* stateblock = 0, psShader* shader = 0, psPass* pass = 0, psInheritable* parent = 0, const psVec& scale = VEC_ONE);
    ~psTileset();
    inline psVeci GetTileDim() const { return _tiledim; }
    void SetTileDim(psVeci tiledim);
    size_t AutoGenDefs(psVec dim);
    size_t AddTileDef(psRect uv, psVec dim, psVec offset = VEC_ZERO);
    bool SetTile(psVeci pos, size_t index, unsigned int color = 0xFFFFFFFF, float rotate = 0, psVec pivot = VEC_ZERO);
    void SetTiles(psTile* tiles, size_t num, size_t pitch);
    inline psVeci GetDimIndex() const { return psVeci(_rowlength, _tiles.Length()/_rowlength); }
    void SetDimIndex(psVeci dim);

    virtual psTex* const* GetTextures() const { return psTextured::GetTextures(); }
    virtual unsigned char NumTextures() const { return psTextured::NumTextures(); }

  protected:
    virtual void _render();

    size_t _rowlength;
    psVeci _tiledim; // Size of the actual tile for figuring out where to put each tile.
    bss_util::cDynArray<psTileDef> _defs; // For each tile indice, stores what the actual UV coordinates of that tile are and what the offset is
    bss_util::cDynArray<psTile> _tiles;
  };
}
#endif