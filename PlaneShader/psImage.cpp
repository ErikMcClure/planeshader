// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psImage.h"

using namespace planeshader;

psImage::psImage(const psImage& copy) : psSolid(copy), psTextured(copy), psColored(copy), _uvs(copy._uvs) {}
psImage::psImage(psImage&& mov) : psSolid(std::move(mov)), psTextured(std::move(mov)), psColored(std::move(mov)), _uvs(std::move(mov._uvs)) {}
psImage::psImage(const DEF_IMAGE& def) : psSolid(def, INTERNALTYPE_IMAGE), psTextured(def), psColored(def), _uvs(def.sources.size()) { for(unsigned char i = 0; i < _uvs.Size(); ++i) _uvs[i] = def.sources[i]; }
psImage::~psImage() {}
psImage::psImage(psTex* tex, const psVec3D& position, FNUM rotation, const psVec& pivot, FLAG_TYPE flags, int zorder, psStateblock* stateblock, psShader* shader, psPass* pass, psInheritable* parent, const psVec& scale, unsigned int color) : 
  psSolid(position, rotation, pivot, flags, zorder, stateblock, shader, pass, parent, scale, INTERNALTYPE_IMAGE), psTextured(tex), psColored(color)
{
  AddSource();
}

psImage::psImage(const char* file, const psVec3D& position, FNUM rotation, const psVec& pivot, FLAG_TYPE flags, int zorder, psStateblock* stateblock, psShader* shader, psPass* pass, psInheritable* parent, const psVec& scale, unsigned int color) :
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

void psImage::AddSource(const psRect& r) { _uvs.Insert(r, _uvs.Size()); }
void psImage::ClearSources() { _uvs.SetSizeDiscard(0); }

void psImage::_setuvs(unsigned int size)
{
  unsigned int oldsize = _uvs.Size();
  if(size>oldsize)
    _uvs.SetSize(size);
  for(unsigned int i = oldsize; i < _uvs.Size(); ++i)
    _uvs[i]=RECT_UNITRECT;
}

void psImage::_render()
{
  _driver->DrawRect(GetCollisionRect(), _uvs, NumSources(), GetColor().color, GetTextures(), NumTextures(), GetAllFlags());
}

void BSS_FASTCALL psImage::_renderbatch(psRenderable** rlist, unsigned int count)
{
  psImage** list = (psImage**)rlist;
  _driver->DrawRectBatchBegin(GetTextures(), NumTextures(), NumSources(), GetAllFlags());

  for(unsigned int i = 0; i < count; ++i)
    _driver->DrawRectBatch(list[i]->GetCollisionRect(), list[i]->_uvs, list[i]->GetColor().color);

  _driver->DrawRectBatchEnd();
}

bool BSS_FASTCALL psImage::_batch(psRenderable* r) const
{
  unsigned char c = NumTextures();
  if(c != r->NumTextures() || GetAllFlags() != r->GetAllFlags() || NumSources() != static_cast<psImage*>(r)->NumSources())
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