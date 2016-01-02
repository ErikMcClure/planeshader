// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psTextured.h"

using namespace planeshader;

psTextured::psTextured(const psTextured& copy) : _tex(copy._tex)
{
  for(unsigned int i = 0; i < _tex.Capacity(); ++i)
    _tex[i]->Grab();
}
psTextured::psTextured(psTextured&& mov) : _tex(std::move(mov._tex)) { }
psTextured::psTextured(const char* file) { SetTexture(psTex::Create(file)); }
psTextured::psTextured(psTex* tex) { SetTexture(tex); }
psTextured::~psTextured() {}

void BSS_FASTCALL psTextured::SetTexture(psTex* tex, unsigned int index)
{
  unsigned int oldsize = _tex.Capacity();
  if(index>=oldsize)
    _tex.SetCapacity(index+1);
  for(unsigned int i = oldsize; i <= index; ++i) // use <= here on purpose
    _tex[i]=0;

  if(_tex[index]) _tex[index]->Drop();
  _tex[index] = tex;
  if(_tex[index]) _tex[index]->Grab();
}
void BSS_FASTCALL psTextured::ClearTextures() { _tex.Clear(); }

psTextured& psTextured::operator=(const psTextured& right)
{
  _tex = right._tex;

  for(unsigned int i = 0; i < _tex.Capacity(); ++i)
    _tex[i]->Grab();

  return *this;
}
psTextured& psTextured::operator=(psTextured&& right)
{
  _tex = std::move(right._tex);
  return *this;
}
