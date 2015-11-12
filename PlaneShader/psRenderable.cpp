// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psRenderable.h"
#include "psEngine.h"
#include "psPass.h"
#include "psStateblock.h"
#include "psShader.h"
#include "bss-util/profiler.h"

using namespace planeshader;

psRenderable::psRenderable(const psRenderable& copy) : _flags(copy._flags), _pass(0), _internalflags(copy._internalflags), _zorder(copy._zorder),
  _stateblock(copy._stateblock), _shader(psShader::CreateShader(copy._shader))
{
  _llist.next=_llist.prev=0;
  SetPass(copy._pass);
}
psRenderable::psRenderable(psRenderable&& mov) : _flags(mov._flags), _pass(0), _internalflags(mov._internalflags), _zorder(mov._zorder),
  _stateblock(std::move(mov._stateblock)), _shader(mov._shader)
{ 
  _llist.next=_llist.prev=0;
  psPass* p = mov._pass;
  mov.SetPass(0);
  SetPass(p);
  mov._shader=0;
}

psRenderable::psRenderable(const DEF_RENDERABLE& def, unsigned char internaltype) : _flags(def.flags), _zorder(def.zorder), _pass(0), _internalflags(internaltype),
  _stateblock(def.stateblock), _shader(psShader::CreateShader(def.shader))
{ 
  _llist.next=_llist.prev=0;
  SetPass(def.pass); 
}

psRenderable::psRenderable(FLAG_TYPE flags, int zorder, psStateblock* stateblock, psShader* shader, psPass* pass, unsigned char internaltype) : _flags(flags), _zorder(zorder),
  _stateblock(stateblock), _shader(psShader::CreateShader(shader)), _pass(0), _internalflags(internaltype), _psort(0)
{ 
  _llist.next=_llist.prev=0;
  SetPass(pass);
}

psRenderable::~psRenderable() { _destroy(); }
void psRenderable::Render()
{
  if(psPass::CurPass != 0)
    psPass::CurPass->_queue(this); // Get the current pass, and add this to it's render queue.
  else
    _render(); // Otherwise, render this immediately
}

void BSS_FASTCALL psRenderable::SetPass()
{
  SetPass(psEngine::Instance()->GetPass(0));
}
void BSS_FASTCALL psRenderable::SetPass(psPass* pass)
{
  PROFILE_FUNC();
  if(pass == _pass) return;
  //if(_pinfo) _pinfo->p=0;
  //_pinfo=0;
  if(_pass != 0)
    _pass->Remove(this);
  
  if(pass)
    pass->Insert(this); // This sets _pass for us
}

void BSS_FASTCALL psRenderable::SetStateblock(psStateblock* stateblock)
{
  PROFILE_FUNC();
  _stateblock = stateblock;
  _invalidate();
}

FLAG_TYPE psRenderable::GetAllFlags() const { return GetFlags(); }
psTex* const* psRenderable::GetTextures() const { return 0; } // these aren't inline because they're virtual
unsigned char psRenderable::NumTextures() const { return 0; }
psTex* const* psRenderable::GetRenderTargets() const { return 0; }
unsigned char psRenderable::NumRT() const { return 0; }
//char psRenderable::_sort(psRenderable* r) const { return SGNCOMPARE(this, r); }
bool psRenderable::_batch(psRenderable* r) const { return false; }
void psRenderable::_renderbatch() { _render(); }
void psRenderable::_renderbatchlist(psRenderable** rlist, unsigned int count) { for(unsigned int i = 0; i < count; ++i) rlist[i]->_render(); }

void psRenderable::_destroy()
{
  SetPass(0);
  if(_shader) _shader->Drop();
}

void psRenderable::_invalidate()
{
  if(_pass != 0)
  {
    if(_psort!=0 && ((_psort->prev!=0 && psPass::StandardCompare(_psort->prev->value, this)>0) || (_psort->next && psPass::StandardCompare(this, _psort->next->value)<0)))
    { // We only invalidate if the new parameters actually invalidate the object's position.
      _pass->_renderlist.Remove(_psort);
      _psort = 0;
    }
  }
}

psRenderable& psRenderable::operator =(const psRenderable& right)
{
  PROFILE_FUNC();
  _destroy();
  _flags=right._flags;
  _internalflags=right._internalflags;
  _zorder=right._zorder;
  _stateblock=right._stateblock;
  _shader=psShader::CreateShader(right._shader);

  _llist.next=_llist.prev=0;
  SetPass(right._pass);
  return *this;
}

psRenderable& psRenderable::operator =(psRenderable&& right)
{
  PROFILE_FUNC();
  _destroy();
  _flags=right._flags;
  _internalflags=right._internalflags;
  _zorder=right._zorder;
  _stateblock=std::move(right._stateblock);
  _shader=right._shader;

  _llist.next=_llist.prev=0;
  psPass* p = right._pass;
  right.SetPass(0);
  SetPass(p);
  right._shader=0;
  return *this;
}

char BSS_FASTCALL psRenderable::_sort(psRenderable* r) const
{
  psRenderable* root = r;
  while(r = r->_getparent()) root = r;

  char c = SGNCOMPARE(_zorder, root->_zorder);
  if(!c) c = SGNCOMPARE(_internaltype(), root->_internaltype());
  if(!c) c = SGNCOMPARE(this, root);
  return c; 
}

psRenderable* BSS_FASTCALL psRenderable::_getparent() const { return 0; }
void BSS_FASTCALL psRenderable::SetRenderTarget(psTex* rt, unsigned int index) {}
