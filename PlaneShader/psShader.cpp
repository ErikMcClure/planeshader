// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psShader.h"
#include "bss-util/profiler.h"

using namespace planeshader;

void psShader::Activate() const
{
  PROFILE_FUNC();
  _driver->SetShader(_ss[0], VERTEX_SHADER_1_1);
  _driver->SetShader(_ss[1], PIXEL_SHADER_1_1);
  _driver->SetShader(_ss[2], GEOMETRY_SHADER_4_0);
  _driver->SetShader(_ss[3], COMPUTE_SHADER_4_0);
  _driver->SetShader(_ss[4], DOMAIN_SHADER_5_0);
  _driver->SetShader(_ss[5], HULL_SHADER_5_0);
  _driver->SetShaderConstants(_sc[0], VERTEX_SHADER_1_1);
  _driver->SetShaderConstants(_sc[1], PIXEL_SHADER_1_1);
  _driver->SetShaderConstants(_sc[2], GEOMETRY_SHADER_4_0); 
  _driver->SetShaderConstants(_sc[3], COMPUTE_SHADER_4_0);
  _driver->SetShaderConstants(_sc[4], DOMAIN_SHADER_5_0);
  _driver->SetShaderConstants(_sc[5], HULL_SHADER_5_0);
  _driver->SetLayout(_layout);
}

bool BSS_FASTCALL psShader::SetConstants(const void* data, uint32_t sz, unsigned char I)
{
  PROFILE_FUNC();
  if(!_sc[I] || sz != _sz[I]) return false;
  void* target=_driver->LockBuffer(_sc[I], LOCK_WRITE_DISCARD);
  if(!target) return false;
  memcpy(target, data, sz);
  _driver->UnlockBuffer(_sc[I]);
  return true;
}

psShader* psShader::CreateShader(unsigned char nlayout, const ELEMENT_DESC* layout, unsigned char num, ...)
{
  PROFILE_FUNC();
  DYNARRAY(SHADER_INFO, infos, num);
  va_list vl;
  va_start(vl, num);
  for(unsigned int i = 0; i < num; ++i) infos[i] = *va_arg(vl, const SHADER_INFO*);
  va_end(vl);
  return CreateShader(nlayout, layout, num, infos);
}
psShader* psShader::CreateShader(unsigned char nlayout, const ELEMENT_DESC* layout, unsigned char num, const SHADER_INFO* infos)
{
  PROFILE_FUNC();
  if(!_driver) return 0;
  void* ss[6]={ 0 };
  void* sc[6]={ 0 };
  uint32_t sz[6]={ 0 };

  unsigned char index=-1;
  unsigned char minvalid=-1;
  unsigned char minindex=-1;
  for(unsigned char i = 0; i < num; ++i)
  {
    if(infos[i].v<=VERTEX_SHADER_5_0)
      index=0;
    else if(infos[i].v<=PIXEL_SHADER_5_0)
      index=1;
    else if(infos[i].v<=GEOMETRY_SHADER_5_0)
      index=2;
    assert(index<6);
    if(minvalid>=index) { minvalid=index; minindex=i; }

    ss[index] = !infos[i].shader?0:_driver->CreateShader(infos[i].shader, infos[i].v);
    sz[index] = infos[i].ty_sz;
    if(sz[index]>0) sc[index] = _driver->CreateBuffer(sz[index], 1, USAGE_CONSTANT_BUFFER|USAGE_DYNAMIC, infos[i].init);
  }
  assert(!num || (minindex<num));
  psShader* s = new psShader((!layout || !infos[minindex].shader)?0:_driver->CreateLayout(infos[minindex].shader, layout, nlayout), ss, sc, sz);
  s->Grab();
  return s;
}
psShader* psShader::CreateShader(const psShader* copy)
{
  PROFILE_FUNC();
  if(!_driver) return 0;
  if(!copy) return CreateShader(_driver->library.IMAGE);

  unsigned char i;
  for(i = 0; i < 6; ++i)
    if(copy->_sc[i] != 0)
      break;

  psShader* r = (i == 6) ? const_cast<psShader*>(copy) : new psShader(*copy); // If i is 6, all _sc values in copy are NULL, and therefore it is stateless.
  r->Grab();
  return r;
}
psShader* BSS_FASTCALL psShader::MergeShaders(unsigned int num, const psShader* first, ...)
{
  PROFILE_FUNC();
  if(!num) return 0;
  psShader* r = new psShader(*first);
  va_list vl;
  va_start(vl, first);
  for(unsigned int i = 1; i < num; ++i)
    *r += *va_arg(vl, const psShader*);
  va_end(vl);
  r->Grab();
  return r;
}
psShader::psShader(const psShader& copy) { _copy(copy); }
psShader::psShader(psShader&& mov) { _move(std::move(mov)); }
psShader::psShader(void* layout, void* ss[6], void* sc[6], uint32_t sz[6]) : _layout(layout)
{
  for(unsigned char i = 0; i < 6; ++i) _ss[i]=ss[i];
  for(unsigned char i = 0; i < 6; ++i) _sc[i]=sc[i];
  for(unsigned char i = 0; i < 6; ++i) _sz[i]=sz[i];
}
psShader::~psShader() { _destroy(); }
void psShader::_destroy()
{
  PROFILE_FUNC();
  for(unsigned char i = 0; i < 6; ++i)
    if(_sc[i]) _driver->FreeResource(_sc[i], psDriver::RES_CONSTBUF);

  for(unsigned char i = 0; i < 6; ++i)
    if(_ss[i]) _driver->FreeResource(_ss[i], (psDriver::RESOURCE_TYPE)(psDriver::RES_SHADERVS+i));

  if(_layout) _driver->FreeResource(_layout, psDriver::RES_LAYOUT);
}
void psShader::_copy(const psShader& copy)
{
  PROFILE_FUNC();
  _layout=copy._layout;
  if(_layout) _driver->GrabResource(_layout, psDriver::RES_LAYOUT);
  for(unsigned char i = 0; i < 6; ++i) _ss[i]=copy._ss[i];
  for(unsigned char i = 0; i < 6; ++i) _sz[i]=copy._sz[i];
  for(unsigned char i = 0; i < 6; ++i)
  {
    _sc[i] = 0;
    if(copy._sc[i])
    {
      _sc[i]=_driver->CreateBuffer(copy._sz[i], 1, USAGE_CONSTANT_BUFFER|USAGE_DYNAMIC, 0);
      _driver->CopyResource(_sc[i], copy._sc[i], psDriver::RES_CONSTBUF);
    }
  }
  for(unsigned char i = 0; i < 6; ++i)
    if(_ss[i]) _driver->GrabResource(_ss[i], (psDriver::RESOURCE_TYPE)(psDriver::RES_SHADERVS+i));
}
void psShader::_move(psShader&& mov)
{
  PROFILE_FUNC();
  _layout=mov._layout;
  mov._layout=0;
  for(unsigned char i = 0; i < 6; ++i) { _ss[i]=mov._ss[i]; mov._ss[i] = 0; }
  for(unsigned char i = 0; i < 6; ++i) { _sc[i]=mov._sc[i]; mov._sc[i] = 0; }
  for(unsigned char i = 0; i < 6; ++i) { _sz[i]=mov._sz[i]; mov._sz[i] = 0; }
}
void psShader::DestroyThis() { delete this; }

psShader& psShader::operator=(const psShader& copy)
{
  _destroy();
  _copy(copy);
  return *this;
}
psShader& psShader::operator=(psShader&& mov)
{
  _destroy();
  _move(std::move(mov));
  return *this;
}
psShader& psShader::operator+=(const psShader& right)
{
  PROFILE_FUNC();
  if(!_ss[0] && right._ss[0]!=0)
  {
    if(_layout) _driver->FreeResource(_layout, psDriver::RES_LAYOUT);
    _layout = right._layout;
    if(_layout) _driver->GrabResource(_layout, psDriver::RES_LAYOUT);
  }
  for(unsigned char i = 0; i < 6; ++i)
    if(right._ss[i]!=0)
    {
      if(_ss[i]) _driver->FreeResource(_ss[i], (psDriver::RESOURCE_TYPE)(psDriver::RES_SHADERVS+i));
      if(_sc[i]) _driver->FreeResource(_sc[i], psDriver::RES_CONSTBUF);
      _sc[i]=0;
      _ss[i]=right._ss[i];
      _sz[i]=right._sz[i];
      _driver->GrabResource(_ss[i], (psDriver::RESOURCE_TYPE)(psDriver::RES_SHADERVS+i));
      if(right._sc[i]!=0)
      {
        _sc[i]=_driver->CreateBuffer(right._sz[i], 1, USAGE_CONSTANT_BUFFER|USAGE_DYNAMIC, 0);
        _driver->CopyResource(_sc[i], right._sc[i], psDriver::RES_CONSTBUF);
      }
    }
  return *this;
}