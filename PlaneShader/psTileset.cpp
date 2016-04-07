// Copyright ©2016 Black Sphere Studios
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
  psSolid(position, rotation, pivot, flags, zorder, stateblock, shader, pass, parent, scale), _rowlength(0), _tiledim(VEC_ZERO)
{
}

psTileset::~psTileset() {}

uint32_t psTileset::AutoGenDefs(psVec dim)
{
  _defs.Clear();
  const psTex* base = GetTexture(0);
  psVeci defs = base->GetDim() / dim;
  _defs.SetLength(defs.x*defs.y);

  for(int j = 0; j < defs.y; ++j)
    for(int i = 0; i < defs.x; ++i)
    {
      psTileDef& def = _defs[i + (j*defs.x)];
      def.level = 0;
      def.rect = psRect { 0, 0, dim.x, dim.y };
      def.uv = (psRect { dim.x, dim.y, dim.x, dim.y }*psRect(i, j, i + 1, j + 1)) / base->GetDim();
    }

  SetTileDim(dim);
  return _defs.Length();
}

uint32_t psTileset::AddTileDef(psRect uv, psVec dim, psVec offset)
{
  psTileDef def = { uv, psRect(offset.x, offset.y, offset.x + dim.x, offset.y + dim.y) };
  return _defs.Add(def);
}

void psTileset::SetTileDim(psVeci tiledim)
{
  _tiledim = tiledim;
  SetDim(!_rowlength ? VEC_ZERO : psVeci(_rowlength, _tiles.Length() / _rowlength)*_tiledim);
}

bool psTileset::SetTile(psVeci pos, uint32_t index, unsigned int color, float rotate, psVec pivot)
{
  uint32_t i = pos.x + pos.y*_rowlength;
  if(i >= _tiles.Length())
    return false;

  _tiles[i].index = index;
  _tiles[i].color = color;
  _tiles[i].rotate = rotate;
  _tiles[i].pivot = pivot;
  return true;
}

void psTileset::SetTiles(psTile* tiles, uint32_t num, uint32_t pitch)
{
  SetDimIndex(psVeci(pitch, num / pitch));
  memset(_tiles.begin(), 0, _tiles.Length()*sizeof(psTile)); // zero everything, which prevents our trailing tiles from showing up in case you didn't give a perfectly square array.
  memcpy(_tiles.begin(), tiles, num*sizeof(psTile)); // copy memory in
}

void psTileset::SetDimIndex(psVeci dim)
{
  _rowlength = dim.x;
  _tiles.SetLength(dim.x * dim.y);
  SetDim(dim*_tiledim);
}

void BSS_FASTCALL psTileset::_render()
{
  assert(_defs.Length() > 0);
  GetTransform(_m);
  Activate();

  psBatchObj& obj = _driver->DrawRectBatchBegin(GetShader(), GetStateblock(), 1, GetAllFlags(), _m.v);

  for(uint32_t i = 0; i < _tiles.Length(); ++i)
  {
    float x = (i%_rowlength) * _tiledim.x;
    float y = (i/_rowlength) * _tiledim.y;
    psTileDef& def = _defs[_tiles[i].index];
    _driver->DrawRectBatch(obj,
      psRectRotateZ(x + def.rect.left, y + def.rect.top, x + def.rect.right, y + def.rect.bottom, _tiles[i].rotate, _tiles[i].pivot, 0),
      &def.uv, 
      _tiles[i].color);
  }
}