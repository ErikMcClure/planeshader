// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psRenderable.h"
#include "psEngine.h"
#include "psPass.h"
#include "psTex.h"
#include "bss-util/profiler.h"

using namespace planeshader;

psRenderable::psRenderable(const psRenderable& copy) : _flags(copy._flags), _pass(0), _internalflags(copy._internalflags), _zorder(copy._zorder),
  _stateblock(copy._stateblock), _shader(psShader::CreateShader(copy._shader)), _rts(copy._rts)
{
  for(unsigned int i = 0; i < _rts.Capacity(); ++i)
    _rts[i]->Grab();
  _llist.next=_llist.prev=0;
  SetPass(copy._pass);
}
psRenderable::psRenderable(psRenderable&& mov) : _flags(mov._flags), _pass(0), _internalflags(mov._internalflags), _zorder(mov._zorder),
  _stateblock(std::move(mov._stateblock)), _shader(std::move(mov._shader)), _rts(std::move(mov._rts))
{ 
  _llist.next=_llist.prev=0;
  psPass* p = mov._pass;
  mov.SetPass(0);
  SetPass(p);
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
    psPass::CurPass->_cullqueue(this); // Get the current pass, and add this to it's render queue, culling if necessary.
  else
  {
    GetShader()->Activate();
    psDriverHold::GetDriver()->SetStateblock(!GetStateblock() ? 0 : GetStateblock()->GetSB());
    psDriverHold::GetDriver()->SetRenderTargets(GetRenderTargets(), NumRT(), 0);
    _render(); // Otherwise, render this immediately
  }
}
void BSS_FASTCALL psRenderable::SetZOrder(int zorder)
{ 
  _zorder = zorder;
  _invalidate();
}
void BSS_FASTCALL psRenderable::SetPass()
{
  SetPass(psEngine::Instance()->GetPass(0));
}
void BSS_FASTCALL psRenderable::SetPass(psPass* pass)
{
  PROFILE_FUNC();
  if(pass == _pass) return;
  if(_pass != 0)
    _pass->Remove(this); // this sets _pass to 0 for us
  
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
inline psTex* const* psRenderable::GetRenderTargets() const
{
  if(_rts.Capacity())
    return _rts;
  return !_pass ? 0 : _pass->GetRenderTarget();
}
inline unsigned char psRenderable::NumRT() const { return !_rts.Capacity() ? (_pass != 0 && _pass->GetRenderTarget()[0] != 0) : _rts.Capacity(); }
void BSS_FASTCALL psRenderable::SetRenderTarget(psTex* rt, unsigned int index)
{
  unsigned int oldsize = _rts.Capacity();
  if(index >= oldsize)
    _rts.SetCapacity(index + 1);
  for(unsigned int i = oldsize; i < index; ++i)
    _rts[i] = 0;
  if(_rts[index]) _rts[index]->Drop();
  _rts[index] = rt;
  if(_rts[index]) _rts[index]->Grab();
}
void psRenderable::ClearRenderTargets() {
  for(unsigned int i = 0; i < _rts.Capacity(); ++i)
    _rts[i]->Drop();
  _rts.Clear();
}
//char psRenderable::_sort(psRenderable* r) const { return SGNCOMPARE(this, r); }
bool psRenderable::_batch(psRenderable* r) const { return false; }
void psRenderable::_renderbatch() { _render(); }
void psRenderable::_renderbatchlist(psRenderable** rlist, unsigned int count) { for(unsigned int i = 0; i < count; ++i) rlist[i]->_render(); }

void psRenderable::_destroy()
{
  SetPass(0);
  ClearRenderTargets();
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
  _rts = right._rts;

  for(unsigned int i = 0; i < _rts.Capacity(); ++i)
    _rts[i]->Grab();

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
  _shader=std::move(right._shader);
  _rts = std::move(right._rts);

  _llist.next=_llist.prev=0;
  psPass* p = right._pass;
  right.SetPass(0);
  SetPass(p);
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
//psCamera* psRenderable::GetCamera() const { return 0; }
