// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psTileset.h"

using namespace planeshader;

psTileset::psTileset(const psTileset& copy) : psSolid(copy), psTextured(copy) {}
psTileset::psTileset(psTileset&& mov) : psSolid(std::move(mov)), psTextured(std::move(mov)) {}
psTileset::psTileset(const DEF_TILESET& def) : psSolid(def), psTextured(def) {}
psTileset::psTileset(const psVec3D& position, FNUM rotation, const psVec& pivot, FLAG_TYPE flags, int zorder, psStateblock* stateblock, psShader* shader, psPass* pass, psInheritable* parent, const psVec& scale) :
  psSolid(position, rotation, pivot, flags, zorder, stateblock, shader, pass, parent, scale, psRenderable::INTERNALTYPE_TILESET)
{

}

void psTileset::_render()
{
  _driver->DrawRectBatchBegin(GetTextures(), NumTextures(), 1, GetAllFlags());
  const psRectRotateZ& rect = GetCollisionRect();
  bss_util::Matrix<float, 4, 4> m;
  bss_util::Matrix<float, 4, 4>::AffineTransform_T(rect.left, rect.top, rect.z, rect.rotation, rect.pivot.x, rect.pivot.y, m);
  sseVec(m.v[0])*sseVec(_scale.x) >> m.v[0];
  sseVec(m.v[1])*sseVec(_scale.y) >> m.v[1];

  for(size_t i = 0; i < _tiles.Length(); ++i)
  {
    float x = (i%_rowlength) * _tiledim.x;
    float y = (i/_rowlength) * _tiledim.y;
    float u = (_tiles[i].indice % _rowlength) * _texdim.x;
    float v = (_tiles[i].indice / _rowlength) * _texdim.y;
    _driver->DrawRectBatch(
      psRectRotateZ(x, y, x + _tiledim.x, y + _tiledim.y, _tiles[i].rotate, _tiles[i].pivot, 0), 
      &psRect(u, v, u + _texdim.x, v + _texdim.y), 
      _tiles[i].color, 
      m.v);
  }

  _driver->DrawRectBatchEnd(m.v);
}