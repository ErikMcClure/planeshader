// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psRenderable.h"
#include "psEngine.h"
#include "psPass.h"
#include "psTex.h"
#include "bss-util/profiler.h"

using namespace planeshader;

psRenderable::psRenderable(const psRenderable& copy) : _flags(copy._flags), _pass(copy._pass), _internalflags(copy._internalflags), _zorder(copy._zorder),
  _stateblock(copy._stateblock), _shader(psShader::CreateShader(copy._shader)), _rts(copy._rts)
{
  for(uint32_t i = 0; i < _rts.Capacity(); ++i)
    _rts[i]->Grab();
  _llist.next=_llist.prev=0;
  _copyinsert(copy);
}
psRenderable::psRenderable(psRenderable&& mov) : _flags(mov._flags), _pass(mov._pass), _internalflags(mov._internalflags), _zorder(mov._zorder),
  _stateblock(std::move(mov._stateblock)), _shader(std::move(mov._shader)), _rts(std::move(mov._rts))
{ 
  _llist.next=_llist.prev=0;
  _copyinsert(mov);

  if(mov._pass != 0) // We deliberately don't call SetPass(0), because we don't WANT to propagate this change down to mov's children - there's no point, because the children aren't being moved!
    mov._pass->Remove(this);
  mov._pass = 0;
}

psRenderable::psRenderable(psFlag flags, int zorder, psStateblock* stateblock, psShader* shader, psPass* pass) : _flags(flags), _zorder(zorder),
  _stateblock(stateblock), _shader(psShader::CreateShader(shader)), _pass(0), _internalflags(0), _psort(0)
{ 
  _llist.next=_llist.prev=0;
  SetPass(pass);
}

psRenderable::~psRenderable() { _destroy(); }
void psRenderable::Render()
{
  if(_pass)
    _pass->_sort(this);
  else
    _render(); // This is a renderable, so it can't be culled, so we just render it immediately and ignore our current pass.
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
    _pass->Remove(this);
  _pass = pass;
}

void BSS_FASTCALL psRenderable::SetStateblock(psStateblock* stateblock)
{
  PROFILE_FUNC();
  _stateblock = stateblock;
  _invalidate();
}

psFlag psRenderable::GetAllFlags() const { return GetFlags(); }
psTex* const* psRenderable::GetTextures() const { return 0; } // these aren't inline because they're virtual
uint8_t psRenderable::NumTextures() const { return 0; }
inline psTex* const* psRenderable::GetRenderTargets() const
{
  if(_rts.Capacity())
    return _rts;
  return !_pass ? 0 : _pass->GetRenderTarget();
}
inline uint8_t psRenderable::NumRT() const { return !_rts.Capacity() ? (_pass != 0 && _pass->GetRenderTarget()[0] != 0) : _rts.Capacity(); }
void BSS_FASTCALL psRenderable::SetRenderTarget(psTex* rt, uint32_t index)
{
  uint32_t oldsize = _rts.Capacity();
  if(index >= oldsize)
    _rts.SetCapacity(index + 1);
  for(uint32_t i = oldsize; i < index; ++i)
    _rts[i] = 0;
  if(_rts[index]) _rts[index]->Drop();
  _rts[index] = rt;
  if(_rts[index]) _rts[index]->Grab();
}
void psRenderable::ClearRenderTargets() {
  for(uint32_t i = 0; i < _rts.Capacity(); ++i)
    _rts[i]->Drop();
  _rts.Clear();
}

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

  for(uint32_t i = 0; i < _rts.Capacity(); ++i)
    _rts[i]->Grab();

  _llist.next=_llist.prev=0;
  _pass = right._pass; // We can do this because _destroy already removed us from any previous pass
  _copyinsert(right);
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
  _pass = right._pass;
  _copyinsert(right);

  if(right._pass != 0) // We should have already removed right's children, but just in case we avoid calling SetPass anyway.
    right._pass->Remove(this);
  right._pass = 0;
  return *this;
}

char BSS_FASTCALL psRenderable::_sort(psRenderable* r) const
{
  psRenderable* root = r;
  while(r = r->_getparent()) root = r;

  char c = SGNCOMPARE(_zorder, root->_zorder);
  if(!c) c = SGNCOMPARE(this, root);
  return c; 
}

psRenderable* BSS_FASTCALL psRenderable::_getparent() const { return 0; }
//psCamera* psRenderable::GetCamera() const { return 0; }

void psRenderable::Activate()
{
  psDriverHold::GetDriver()->SetRenderTargets(GetRenderTargets(), NumRT(), 0);
  psDriverHold::GetDriver()->SetTextures(GetTextures(), NumTextures(), PIXEL_SHADER_1_1);
}

void psRenderable::_copyinsert(const psRenderable& r)
{
  if(r._pass != 0 && (r._llist.prev != 0 || r._pass->_renderables == &r)) // Check if r was in the internal pass list. If so, add ourselves to it.
    r._pass->Insert(this);
}
