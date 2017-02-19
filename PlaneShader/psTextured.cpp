// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psTextured.h"

using namespace planeshader;

psTextured::psTextured(const psTextured& copy) : _tex(copy._tex)
{
  for(uint32_t i = 0; i < _tex.Capacity(); ++i)
    _tex[i]->Grab();
}
psTextured::psTextured(psTextured&& mov) : _tex(std::move(mov._tex)) { }
psTextured::psTextured(const char* file) { SetTexture(psTex::Create(file)); }
psTextured::psTextured(psTex* tex) { SetTexture(tex); }
psTextured::~psTextured() {}

void psTextured::SetTexture(psTex* tex, uint32_t index)
{
  uint32_t oldsize = _tex.Capacity();
  if(index>=oldsize)
    _tex.SetCapacity(index+1);
  for(uint32_t i = oldsize; i <= index; ++i) // use <= here on purpose
    _tex[i]=0;

  if(_tex[index]) _tex[index]->Drop();
  _tex[index] = tex;
  if(_tex[index]) _tex[index]->Grab();
}
void psTextured::ClearTextures() { _tex.Clear(); }

psTextured& psTextured::operator=(const psTextured& right)
{
  _tex = right._tex;

  for(uint32_t i = 0; i < _tex.Capacity(); ++i)
    _tex[i]->Grab();

  return *this;
}
psTextured& psTextured::operator=(psTextured&& right)
{
  _tex = std::move(right._tex);
  return *this;
}
