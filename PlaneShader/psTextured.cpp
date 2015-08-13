// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psTextured.h"

using namespace planeshader;

psTextured::psTextured(const psTextured& copy) : _rts(copy._rts),  _tex(copy._tex)
{
  for(unsigned int i = 0; i < _rts.Size(); ++i)
    _rts[i]->Grab();
  for(unsigned int i = 0; i < _tex.Size(); ++i)
    _tex[i]->Grab();
}
psTextured::psTextured(psTextured&& mov) : _tex(std::move(mov._tex)), _rts(std::move(mov._rts)) { }
psTextured::psTextured(const DEF_TEXTURED& def) { for(size_t i = def.texes.size(); i > 0; --i) SetTexture(def.texes[i-1], i-1); }
psTextured::psTextured(const char* file) { SetTexture(psTex::Create(file)); }
psTextured::psTextured(psTex* tex) { SetTexture(tex); }
psTextured::~psTextured() {}

void BSS_FASTCALL psTextured::SetTexture(psTex* tex, unsigned int index)
{
  unsigned int oldsize = _tex.Size();
  if(index>=oldsize)
    _tex.SetSize(index+1);
  for(unsigned int i = oldsize; i < index; ++i)
    _tex[i]=0;
  if(_tex[index]) _tex[index]->Drop();
  _tex[index] = tex;
  if(_tex[index]) _tex[index]->Grab();
}
void BSS_FASTCALL psTextured::ClearTextures() { _tex.SetSizeDiscard(0); _rts.SetSizeDiscard(0); }
void BSS_FASTCALL psTextured::SetRT(psTex* rt, unsigned int index)
{
  unsigned int oldsize = _rts.Size();
  if(index>=oldsize)
    _rts.SetSize(index+1);
  for(unsigned int i = oldsize; i < index; ++i)
    _rts[i]=0;
  if(_rts[index]) _rts[index]->Drop();
  _rts[index] = rt;
  if(_rts[index]) _rts[index]->Grab();
}

psTextured& psTextured::operator=(const psTextured& right)
{
  _rts = right._rts;
  _tex = right._tex;

  for(unsigned int i = 0; i < _rts.Size(); ++i)
    _rts[i]->Grab();
  for(unsigned int i = 0; i < _tex.Size(); ++i)
    _tex[i]->Grab();

  return *this;
}
psTextured& psTextured::operator=(psTextured&& right)
{
  _rts = std::move(right._rts);
  _tex = std::move(right._tex);
  return *this;
}
