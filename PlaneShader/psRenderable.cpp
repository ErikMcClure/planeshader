// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psRenderable.h"
#include "psEngine.h"
#include "psPass.h"
#include "psStateblock.h"
#include "psShader.h"

using namespace planeshader;

psRenderable::psRenderable(const psRenderable& copy) : _flags(copy._flags), _pass((unsigned short)-1), _internalflags(copy._internalflags), _zorder(copy._zorder),
  _stateblock(copy._stateblock), _shader(psShader::CreateShader(copy._shader))
{
  _llist.next=_llist.prev=0;
  if(_stateblock) 
    _stateblock->Grab();
  SetPass(copy._pass);
}
psRenderable::psRenderable(psRenderable&& mov) : _flags(mov._flags), _pass((unsigned short)-1), _internalflags(mov._internalflags), _zorder(mov._zorder),
  _stateblock(mov._stateblock), _shader(mov._shader)
{ 
  _llist.next=_llist.prev=0;
  unsigned short p = mov._pass;
  mov.SetPass(-1);
  SetPass(p);
  mov._shader=0;
  mov._stateblock=0;
}

psRenderable::psRenderable(const DEF_RENDERABLE& def) : _flags(def.flags), _zorder(def.zorder), _pass((unsigned short)-1), _internalflags(0),
  _stateblock(def.stateblock), _shader(psShader::CreateShader(def.shader))
{ 
  _llist.next=_llist.prev=0;
  if(_stateblock)
    _stateblock->Grab();
  SetPass(def.pass); 
}
psRenderable::psRenderable(FLAG_TYPE flags, int zorder, psStateblock* stateblock, psShader* shader, unsigned short pass) : _flags(flags), _zorder(zorder),
  _stateblock(stateblock), _shader(psShader::CreateShader(shader)), _pass((unsigned short)-1)
{ 
  _llist.next=_llist.prev=0;
  if(_stateblock)
    _stateblock->Grab();
  SetPass(pass);
}
psRenderable::~psRenderable() { _destroy(); }
void BSS_FASTCALL psRenderable::SetPass(unsigned short pass)
{
  if(pass==_pass) return;
  //if(_pinfo) _pinfo->p=0;
  //_pinfo=0;
  if(_pass!=((unsigned short)-1))
    psEngine::Instance()->GetPass(_pass)->Remove(this);
  _pass=pass;
  if(pass==(unsigned short)-1) return;

  if(pass>=psEngine::Instance()->NumPass())
  {
    PSLOG(2) << pass << "is an invalid pass index" << std::endl;
    pass=(unsigned short)-1;
  }
  else
    psEngine::Instance()->GetPass(_pass)->Insert(this);
}
void BSS_FASTCALL psRenderable::SetStateblock(psStateblock* stateblock)
{
  if(_stateblock)
    _stateblock->Drop();
  _stateblock = stateblock;
  if(_stateblock)
    _stateblock->Grab();
}
psTex* const* psRenderable::GetTextures() const { return 0; } // these aren't inline because they're virtual
unsigned char psRenderable::NumTextures() const { return 0; }
psTex* const* psRenderable::GetRenderTargets() const { return 0; }
unsigned char psRenderable::NumRT() const { return 0; }

void psRenderable::_destroy()
{
  SetPass((unsigned short)-1);
  if(_stateblock) _stateblock->Drop();
  if(_shader) _shader->Drop();
}
psRenderable& psRenderable::operator =(const psRenderable& right)
{
  _destroy();
  _flags=right._flags;
  _internalflags=right._internalflags;
  _zorder=right._zorder;
  _stateblock=right._stateblock;
  _shader=psShader::CreateShader(right._shader);

  _llist.next=_llist.prev=0;
  if(_stateblock)
    _stateblock->Grab();
  SetPass(right._pass);
  return *this;
}
psRenderable& psRenderable::operator =(psRenderable&& right)
{
  _destroy();
  _flags=right._flags;
  _internalflags=right._internalflags;
  _zorder=right._zorder;
  _stateblock=right._stateblock;
  _shader=right._shader;

  _llist.next=_llist.prev=0;
  unsigned short p = right._pass;
  right.SetPass(-1);
  SetPass(p);
  right._shader=0;
  right._stateblock=0;
  return *this;
}

