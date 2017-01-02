// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psImage.h"
#include "psPass.h"

using namespace planeshader;

psImage::psImage(const psImage& copy) : psSolid(copy), psTextured(copy), psColored(copy), _uvs(copy._uvs) {}
psImage::psImage(psImage&& mov) : psSolid(std::move(mov)), psTextured(std::move(mov)), psColored(std::move(mov)), _uvs(std::move(mov._uvs)) {}
psImage::~psImage() {}
psImage::psImage(psTex* tex, const psVec3D& position, FNUM rotation, const psVec& pivot, psFlag flags, int zorder, psStateblock* stateblock, psShader* shader, psPass* pass, psInheritable* parent, const psVec& scale, uint32_t color) : 
  psSolid(position, rotation, pivot, flags, zorder, stateblock, shader, pass, parent, scale), psTextured(tex), psColored(color)
{
  AddSource();
}

psImage::psImage(const char* file, const psVec3D& position, FNUM rotation, const psVec& pivot, psFlag flags, int zorder, psStateblock* stateblock, psShader* shader, psPass* pass, psInheritable* parent, const psVec& scale, uint32_t color) :
  psSolid(position, rotation, pivot, flags, zorder, stateblock, shader, pass, parent, scale), psTextured(file), psColored(color)
{
  AddSource();
}

psImage& psImage::operator =(const psImage& right)
{
  psSolid::operator=(right);
  psColored::operator=(right);
  psTextured::operator=(right);
  _uvs = right._uvs;
  return *this;
}

psImage& psImage::operator =(psImage&& right)
{
  psSolid::operator=(std::move(right));
  psColored::operator=(std::move(right));
  psTextured::operator=(std::move(right));
  _uvs = std::move(right._uvs);
  return *this;
}

void psImage::AddSource(const psRect& r) { _uvs.Insert(r, _uvs.Capacity()); if(_uvs.Capacity()==1) _recalcdim(); }
void psImage::ClearSources() { _uvs.SetCapacityDiscard(0); }

void psImage::_setuvs(uint32_t size)
{
  uint32_t oldsize = _uvs.Capacity();
  if(size>oldsize)
    _uvs.SetCapacity(size);
  for(uint32_t i = oldsize; i < _uvs.Capacity(); ++i)
    _uvs[i]=RECT_UNITRECT;
}

void BSS_FASTCALL psImage::_render()
{
  Activate();
  _driver->DrawRect(GetShader(), GetStateblock(), GetCollisionRect(), _uvs, NumSources(), GetColor().color, GetAllFlags());
}

void BSS_FASTCALL psImage::SetTexture(psTex* tex, uint32_t index)
{
  psTextured::SetTexture(tex, index);
  if(!index) _recalcdim();
}

void psImage::_recalcdim()
{
  if(_tex.Capacity() > 0 && _tex[0])
    SetDim((_uvs.Capacity()>0)?(_tex[0]->GetDim()*_uvs[0].GetDimensions()):_tex[0]->GetDim());
}

void psImage::ApplyEdgeBuffer()
{
  if(_tex.Capacity() > 0 && _tex[0])
  {
    if(!NumSources()) AddSource();
    SetSource(GetSource(0).Inflate(1.0f));
    // This adjusts the pivot by shrinking our new pivot (which is now incorrect) by 1 pixel on each side. This maps (0,0) to (1,1) and (corner,corner) to (corner-1, corner-1), but if the pivot is in the exact center, doesn't change it.
    psVec halfdim = GetDim() / 2.0f;
    SetPivot(((GetPivot() - halfdim) * ((halfdim - VEC_ONE) / halfdim)) + halfdim);
  }
}