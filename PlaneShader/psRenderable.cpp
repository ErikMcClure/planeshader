// Copyright ©2014 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psRenderable.h"
#include "psEngine.h"
#include "psPass.h"

using namespace planeshader;

psRenderable::psRenderable(const psRenderable& copy){}
psRenderable::psRenderable(psRenderable&& mov){}
psRenderable::psRenderable(const DEF_RENDERABLE& def) : _flags(def.flags), _zorder(def.zorder), _pass(-1) { _llist.next=_llist.prev=0; SetPass(def.pass); }
psRenderable::psRenderable(FLAG_TYPE flags, int zorder, unsigned short pass) : _flags(flags), _zorder(zorder), _pass(-1) { _llist.next=_llist.prev=0; SetPass(pass); }
psRenderable::~psRenderable(){}
void BSS_FASTCALL psRenderable::SetPass(unsigned short pass)
{
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
void BSS_FASTCALL psRenderable::SetStateBlock(psStateBlock* stateblock)
{

}

