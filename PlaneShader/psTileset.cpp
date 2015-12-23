// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psTileset.h"

using namespace planeshader;

psTileset::psTileset(const psTileset& copy) : psSolid(copy), psTextured(copy), _defs(copy._defs), _tiles(copy._tiles), _rowlength(copy._rowlength), _tiledim(copy._tiledim) {}

psTileset::psTileset(psTileset&& mov) : psSolid(std::move(mov)), psTextured(std::move(mov)), _defs(std::move(mov._defs)),
  _tiles(std::move(mov._tiles)), _rowlength(mov._rowlength), _tiledim(mov._tiledim)
{
  mov._tiledim = VEC_ZERO;
  mov._rowlength = 0;
}

psTileset::psTileset(const psVec3D& position, FNUM rotation, const psVec& pivot, psFlag flags, int zorder, psStateblock* stateblock, psShader* shader, psPass* pass, psInheritable* parent, const psVec& scale) :
  psSolid(position, rotation, pivot, flags, zorder, stateblock, shader, pass, parent, scale, psRenderable::INTERNALTYPE_TILESET), _rowlength(0), _tiledim(VEC_ZERO)
{
}

psTileset::~psTileset() {}

size_t psTileset::AutoGenDefs(psVec dim)
{
  _defs.Clear();

  return _defs.Length();
}

size_t psTileset::AddTileDef(psRect uv, psVec dim, psVec offset)
{
  psTileDef def = { uv, psRect(offset.x, offset.y, offset.x + dim.x, offset.y + dim.y) };
  return _defs.Add(def);
}

void psTileset::SetTileDim(psVeci tiledim)
{
  _tiledim = tiledim;
  SetDim(psVeci(_rowlength, _tiles.Length()/_rowlength)*_tiledim);
}

bool psTileset::SetTile(psVeci pos, size_t index, unsigned int color, float rotate, psVec pivot)
{
  size_t i = pos.x + pos.y*_rowlength;
  if(i >= _tiles.Length())
    return false;

  _tiles[i].index = index;
  _tiles[i].color = color;
  _tiles[i].rotate = rotate;
  _tiles[i].pivot = pivot;
  return true;
}

void psTileset::SetTiles(psTile* tiles, size_t num, size_t pitch)
{
  SetDimIndex(psVeci(pitch, num / pitch));
  memset(_tiles.begin(), 0, _tiles.Length()*sizeof(psTile)); // zero everything, which prevents our trailing tiles from showing up in case you didn't give a perfectly square array.
  memcpy(_tiles.begin(), tiles, num); // copy memory in
}

void psTileset::SetDimIndex(psVeci dim)
{
  _rowlength = dim.x;
  _tiles.SetLength(dim.x * dim.y);
  SetDim(dim*_tiledim);
}

void BSS_FASTCALL psTileset::_render(psBatchObj*)
{
  const psRectRotateZ& rect = GetCollisionRect();
  bss_util::Matrix<float, 4, 4> m;
  bss_util::Matrix<float, 4, 4>::AffineTransform_T(rect.left, rect.top, rect.z, rect.rotation, rect.pivot.x, rect.pivot.y, m);
  sseVec(m.v[0])*sseVec(_scale.x) >> m.v[0];
  sseVec(m.v[1])*sseVec(_scale.y) >> m.v[1];

  Activate(this); // Sets all the textures and shaders
  psBatchObj obj;
  _driver->DrawRectBatchBegin(obj, 1, GetAllFlags(), m.v);

  for(size_t i = 0; i < _tiles.Length(); ++i)
  {
    float x = (i%_rowlength) * _tiledim.x;
    float y = (i/_rowlength) * _tiledim.y;
    psTileDef& def = _defs[_tiles[i].index];
    _driver->DrawRectBatch(obj,
      psRectRotateZ(x + def.rect.left, y + def.rect.top, x + def.rect.right, y + def.rect.bottom, _tiles[i].rotate, _tiles[i].pivot, 0),
      &def.uv,
      1,
      _tiles[i].color);
  }

  _driver->DrawBatchEnd(obj);
}