// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psImage.h"
#include "psPass.h"

using namespace planeshader;

psImage::psImage(const psImage& copy) : psSolid(copy), psTextured(copy), psColored(copy), _uvs(copy._uvs) {}
psImage::psImage(psImage&& mov) : psSolid(std::move(mov)), psTextured(std::move(mov)), psColored(std::move(mov)), _uvs(std::move(mov._uvs)) {}
psImage::~psImage() {}
psImage::psImage(psTex* tex, const psVec3D& position, FNUM rotation, const psVec& pivot, psFlag flags, int zorder, psStateblock* stateblock, psShader* shader, psPass* pass, psInheritable* parent, const psVec& scale, unsigned int color) : 
  psSolid(position, rotation, pivot, flags, zorder, stateblock, shader, pass, parent, scale, INTERNALTYPE_IMAGE), psTextured(tex), psColored(color)
{
  AddSource();
}

psImage::psImage(const char* file, const psVec3D& position, FNUM rotation, const psVec& pivot, psFlag flags, int zorder, psStateblock* stateblock, psShader* shader, psPass* pass, psInheritable* parent, const psVec& scale, unsigned int color) :
  psSolid(position, rotation, pivot, flags, zorder, stateblock, shader, pass, parent, scale, INTERNALTYPE_IMAGE), psTextured(file), psColored(color)
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

void psImage::_setuvs(unsigned int size)
{
  unsigned int oldsize = _uvs.Capacity();
  if(size>oldsize)
    _uvs.SetCapacity(size);
  for(unsigned int i = oldsize; i < _uvs.Capacity(); ++i)
    _uvs[i]=RECT_UNITRECT;
}

void BSS_FASTCALL psImage::_render(psBatchObj* obj)
{
  if(obj)
    _driver->DrawRectBatch(*obj, GetCollisionRect(), _uvs, NumSources(), GetColor().color);
  else
  {
    Activate(this);
    _driver->DrawRect(GetCollisionRect(), _uvs, NumSources(), GetColor().color, GetAllFlags());
  }
}

void BSS_FASTCALL psImage::_renderbatch(psRenderable** rlist, unsigned int count)
{
  psBatchObj obj;
  Activate(this);
  _driver->DrawRectBatchBegin(obj, NumSources(), GetAllFlags());

  for(unsigned int i = 0; i < count; ++i)
    rlist[i]->_render(&obj);

  _driver->DrawBatchEnd(obj);
}

bool BSS_FASTCALL psImage::_batch(psRenderable* r) const
{
  unsigned char c = NumTextures();
  unsigned char rtc = NumRT();
  if(c != r->NumTextures() || (GetAllFlags()&PSFLAG_BATCHFLAGS) != (r->GetAllFlags()&PSFLAG_BATCHFLAGS)
    || NumSources() != static_cast<psImage*>(r)->NumSources())
    return false;

  psTex* const* t = GetTextures();
  psTex* const* o = r->GetTextures();
  for(unsigned char i = 0; i < c; ++i)
  {
    if(t[i] != o[i])
      return false;
  }

  return true;
}

void BSS_FASTCALL psImage::SetTexture(psTex* tex, unsigned int index)
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