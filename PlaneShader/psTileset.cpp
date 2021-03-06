// Copyright �2018 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#include "psEngine.h"
#include "psTileset.h"

using namespace planeshader;

psTileset::psTileset(const psTileset& copy) : psSolid(copy), psTextured(copy), _defs(copy._defs), _tiles(copy._tiles), _rowlength(copy._rowlength), _tiledim(copy._tiledim) {}

psTileset::psTileset(psTileset&& mov) : psSolid(std::move(mov)), psTextured(std::move(mov)), _defs(std::move(mov._defs)),
  _tiles(std::move(mov._tiles)), _rowlength(mov._rowlength), _tiledim(mov._tiledim)
{
  mov._tiledim = VEC_ZERO;
  mov._rowlength = 0;
}

psTileset::psTileset(const psVec3D& position, FNUM rotation, const psVec& pivot, psFlag flags, int zorder, psStateblock* stateblock, psShader* shader, psLayer* pass, const psVec& scale) :
  psSolid(position, rotation, pivot, flags, zorder, stateblock, shader, pass, scale), _rowlength(0), _tiledim(VEC_ZERO)
{
}

psTileset::~psTileset() {}

psTileset& psTileset::operator=(const psTileset& copy)
{
  psSolid::operator=(copy);
  psTextured::operator=(copy);

  _rowlength = copy._rowlength;
  _tiledim = copy._tiledim;
  _defs = copy._defs;
  _tiles = copy._tiles;
  return *this;
}
psTileset& psTileset::operator=(psTileset&& mov)
{
  psSolid::operator=(std::move(mov));
  psTextured::operator=(std::move(mov));

  _rowlength = mov._rowlength;
  _tiledim = mov._tiledim;
  _defs = std::move(mov._defs);
  _tiles = std::move(mov._tiles);
  mov._tiledim = VEC_ZERO;
  mov._rowlength = 0;
  return *this;
}

uint32_t psTileset::AutoGenDefs(psVec dim)
{
  _defs.Clear();
  const psTex* base = GetTexture(0);
  psVeci defs = !base ? psVeci(0,0) : (base->GetDim() / dim);
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

uint32_t psTileset::AddTileDef(psRect uv, psVec dim, psVec offset, int level)
{
  psTileDef def = { uv, psRect(offset.x, offset.y, offset.x + dim.x, offset.y + dim.y), level };
  return _defs.Add(def);
}

void psTileset::SetTileDim(psVeci tiledim)
{
  _tiledim = tiledim;
  SetDim(!_rowlength ? VEC_ZERO : psVeci(_rowlength, _tiles.Length() / _rowlength)*_tiledim);
}

bool psTileset::SetTile(psVeci pos, uint32_t index, uint32_t color, float rotate, psVec pivot)
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
  Clear();
  memcpy(_tiles.begin(), tiles, num*sizeof(psTile)); // copy memory in
}

void psTileset::Clear()
{
  bss::bssFillN<psTile>(_tiles.begin(), _tiles.Length(), 0);
}

void psTileset::SetDimIndex(psVeci dim)
{
  _rowlength = dim.x;
  _tiles.SetLength(dim.x * dim.y);
  SetDim(dim*_tiledim);
}

void psTileset::_render(const psTransform2D& parent)
{
  assert(_defs.Length() > 0);
  if(!_rowlength || !_tiles.Length()) return;
  psMatrix m;
  GetMatrix(m, &parent);
  _driver->PushTransform(m);
  _driver->SetTextures(GetTextures(), NumTextures(), PIXEL_SHADER_1_1);

  psBatchObj* obj = _driver->DrawRectBatchBegin(GetShader(), GetStateblock(), 1, GetFlags());

  psRecti window = psRecti(0, 0, _rowlength, _tiles.Length() / _rowlength);
  psRectRotateZ crect = GetCollisionRect(parent);
  if(crect.rotation == 0.0f)
  {
    psRect r = crect.BuildAABB();
    psRect c = _driver->PeekClipRect();
    psVec3D lt = _driver->FromScreenSpace(c.topleft, crect.z);
    psVec3D rb = _driver->FromScreenSpace(c.bottomright, crect.z);
    r = psRect(lt.x - r.left, lt.y - r.top, rb.x - r.left, rb.y - r.top);
    window = window.Intersection(psRecti(
      bss::fFastTruncate(r.left / _tiledim.x), 
      bss::fFastTruncate(r.top / _tiledim.y), 
      bss::fFastTruncate(r.right / _tiledim.x) + 1, 
      bss::fFastTruncate(r.bottom / _tiledim.y) + 1));
  }

  uint32_t skipped = 0;
  uint32_t bytecount = T_NEXTMULTIPLE(_tiles.Length(), 7) >> 3;
  assert(bytecount < 0xFFFF);
  VARARRAY(uint8_t, drawn, bytecount);
  memset(drawn, 0, bytecount);

  do
  {
    for(uint32_t j = window.top; j < (uint32_t)window.bottom; ++j)
      for(uint32_t i = window.left; i < (uint32_t)window.right; ++i)
      {
        uint32_t k = i + (j*_rowlength);
        if(bss::bssGetBit<uint8_t>(drawn, k) || _tiles[k].index > _defs.Length() || _tiles[k].color == 0)
          continue; // if we drew this tile already don't draw it again

        psTileDef& def = _defs[_tiles[k].index];
        if(_drawcheck<uint8_t>(drawn, k + 1, def.level) ||  // There are four tiles that we must check the levels of in case they need to render first
          _drawcheck<uint8_t>(drawn, k + _rowlength - 1, def.level) ||
          _drawcheck<uint8_t>(drawn, k + _rowlength + 0, def.level) ||
          _drawcheck<uint8_t>(drawn, k + _rowlength + 1, def.level))
        {
          ++skipped;
          continue;
        }

        float x = i * _tiledim.x; //(k%_rowlength)
        float y = j * _tiledim.y; //(k/_rowlength)
        _driver->DrawRectBatch(obj,
          psRectRotateZ(x + def.rect.left, y + def.rect.top, x + def.rect.right, y + def.rect.bottom, _tiles[k].rotate, _tiles[k].pivot, 0),
          &def.uv,
          _tiles[k].color);
        drawn[k >> 3] |= (1 << (k % 8)); // mark tile as drawn
      }
  } while(skipped > 0);

  _driver->PopTransform();
}