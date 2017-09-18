// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#include "psImage.h"
#include "psLayer.h"

using namespace planeshader;

psImage::psImage(const psImage& copy) : psSolid(copy), psTextured(copy), _color(copy._color), _uvs(copy._uvs) {}
psImage::psImage(psImage&& mov) : psSolid(std::move(mov)), psTextured(std::move(mov)), _color(mov._color), _uvs(std::move(mov._uvs)) {}
psImage::~psImage() {}
psImage::psImage(psTex* tex, const psVec3D& position, FNUM rotation, const psVec& pivot, psFlag flags, int zorder, psStateblock* stateblock, psShader* shader, psLayer* pass, const psVec& scale, uint32_t color) : 
  psSolid(position, rotation, pivot, flags, zorder, stateblock, shader, pass, scale), psTextured(tex), _color(color)
{
  AddSource();
}

psImage::psImage(const char* file, const psVec3D& position, FNUM rotation, const psVec& pivot, psFlag flags, int zorder, psStateblock* stateblock, psShader* shader, psLayer* pass, const psVec& scale, uint32_t color) :
  psSolid(position, rotation, pivot, flags, zorder, stateblock, shader, pass, scale), psTextured(file), _color(color)
{
  AddSource();
}

psImage& psImage::operator =(const psImage& right)
{
  psSolid::operator=(right);
  psTextured::operator=(right);
  _color = right._color;
  _uvs = right._uvs;
  return *this;
}

psImage& psImage::operator =(psImage&& right)
{
  psSolid::operator=(std::move(right));
  psTextured::operator=(std::move(right));
  _color = right._color;
  _uvs = std::move(right._uvs);
  return *this;
}

void psImage::AddSource(const psRect& r) { _uvs.Insert(r, _uvs.Length()); if(_uvs.Length()==1) _recalcdim(); }
void psImage::ClearSources() { _uvs.SetLength(0); }

void psImage::_setuvs(size_t size)
{
  uint32_t oldsize = _uvs.Length();
  if(size>oldsize)
    _uvs.SetLength(size);
  for(uint32_t i = oldsize; i < _uvs.Length(); ++i)
    _uvs[i]=RECT_UNITRECT;
}

void psImage::_render(const psTransform2D& parent)
{
  _driver->SetTextures(GetTextures(), NumTextures(), PIXEL_SHADER_1_1);
  _driver->DrawRect(GetShader(), GetStateblock(), GetCollisionRect(parent), _uvs, NumSources(), GetColor().color, GetFlags());
}

void psImage::SetTexture(psTex* tex, size_t index)
{
  psTextured::SetTexture(tex, index);
  if(!index) _recalcdim();
}

void psImage::_recalcdim()
{
  if(_tex.Length() > 0 && _tex[0])
    SetDim((_uvs.Length()>0)?(_tex[0]->GetDim()*_uvs[0].Dim()):_tex[0]->GetDim());
}

void psImage::ApplyEdgeBuffer()
{
  if(_tex.Length() > 0 && _tex[0])
  {
    if(!NumSources()) AddSource();
    SetSource(GetSource(0).Inflate(1.0f));
    // This adjusts the pivot by shrinking our new pivot (which is now incorrect) by 1 pixel on each side. This maps (0,0) to (1,1) and (corner,corner) to (corner-1, corner-1), but if the pivot is in the exact center, doesn't change it.
    psVec halfdim = GetDim() / 2.0f;
    SetPivot(((GetPivot() - halfdim) * ((halfdim - VEC_ONE) / halfdim)) + halfdim);
  }
}