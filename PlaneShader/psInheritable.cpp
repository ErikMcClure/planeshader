// Copyright ©2016 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psInheritable.h"
#include "bss-util/profiler.h"
#include "psPass.h"
#include <algorithm>

using namespace planeshader;

psInheritable::psInheritable(const psInheritable& copy) : psRenderable(copy), psLocatable(copy), _parent(0), _children(0), _depth(0)
{
  _lchild.next=_lchild.prev=0;
  SetParent(copy._parent);
  for(psInheritable* cur = _children; cur != 0; cur = cur->_lchild.next)
    cur->Clone()->SetParent(this, true);
}
psInheritable::psInheritable(psInheritable&& mov) : psRenderable(std::move(mov)), psLocatable(std::move(mov)), _parent(mov._parent), _children(mov._children), _depth(mov._depth), _lchild(mov._lchild)
{
  mov._parent = 0;
  mov._children = 0;
  mov._lchild.next= mov._lchild.prev=0;
  mov._depth = 0;
  
  if(_lchild.next) _lchild.next->_lchild.prev = this; // Fix the linked list
  if(_lchild.prev) _lchild.prev->_lchild.next = this;
  else _parent->_children = this;

  for(psInheritable* cur=_children; cur!=0; cur=cur->_lchild.next)
    cur->_parent=this;
  SetPass(_pass); // psRenderable won't have been able to resolve this virtual function in its constructor, so we call it again, which will fix up the passes for the children
}
psInheritable::psInheritable(const psVec3D& position, FNUM rotation, const psVec& pivot, psFlag flags, int zorder, psStateblock* stateblock, psShader* shader, psPass* pass, psInheritable* parent) :
  psRenderable(flags, zorder, stateblock, shader, pass), psLocatable(position, rotation, pivot), _parent(0), _children(0), _depth(0)
{
  _lchild.next=_lchild.prev=0;
  SetParent(parent);
}
psInheritable::~psInheritable()
{
  while(_children != 0)
  {
    if(_children->_internalflags&INTERNALFLAG_OWNED)
      delete _children;
    else
      _children->SetParent(0);
  }

  SetParent(0);
}
psInheritable* psInheritable::_prerender()
{
  if(!(_internalflags&INTERNALFLAG_SORTED))
    _sortchildren();

  psInheritable* cur;
  for(cur = _children; cur != 0 && cur->_zorder<0; cur = cur->_lchild.next)
    cur->Render();
  return cur;
}
void psInheritable::Render()
{
  psInheritable* cur = _prerender();
  psRenderable::Render();
  for(; cur != 0; cur = cur->_lchild.next)
    cur->Render();
}
void psInheritable::SetParent(psInheritable* parent, bool ownership)
{
  PROFILE_FUNC();
  if(parent==_parent) return;
  if(_parent)
    bss_util::AltLLRemove<psInheritable,&GetLLBase>(this, _parent->_children);

  _lchild.next=_lchild.prev=0;
  _parent=parent;
  if(_parent==this) // No circular reference insanity allowed!
    _parent=0;

  _internalflags &= (~INTERNALFLAG_OWNED);
  if(_parent!=0)
  {
    SetPass(_parent->_pass);
    bss_util::AltLLAdd<psInheritable, &GetLLBase>(this, _parent->_children);
    _parent->_internalflags &= ~INTERNALFLAG_SORTED; // No longer sorted
    _depth = _parent->_depth + 1;
    if(ownership) _internalflags |= INTERNALFLAG_OWNED;
  }
  _invalidate();
}

psInheritable* BSS_FASTCALL psInheritable::AddClone(const psInheritable* inheritable)
{
  psInheritable* child = inheritable->Clone();
  child->SetParent(this, true);
  return child;
}

void BSS_FASTCALL psInheritable::SetPass(psPass* pass)
{
  PROFILE_FUNC();
  psRenderable::SetPass(pass);
  for(psInheritable* cur=_children; cur!=0; cur=cur->_lchild.next)
    cur->SetPass(pass);
}
void BSS_FASTCALL psInheritable::SetZOrder(int zorder)
{
  psRenderable::SetZOrder(zorder);
  for(psInheritable* cur = _children; cur != 0; cur = cur->_lchild.next)
    cur->_invalidate();
}

void BSS_FASTCALL psInheritable::_render() {} // don't render anything

void psInheritable::_gettotalpos(psVec3D& pos) const
{
  PROFILE_FUNC();
  if(_rotation != 0.0f)
    psVec::RotatePoint(pos.x, pos.y, GetTotalRotation(), pos.x, pos.y);
  pos += _relpos;
  if(_parent)
    _parent->_gettotalpos(pos);
}
psInheritable& psInheritable::operator=(const psInheritable& copy)
{
  PROFILE_FUNC();
  while(_children!=0) _children->SetParent(0);
  SetParent(copy._parent);
  psRenderable::operator=(copy);
  psLocatable::operator=(copy);
  return *this;
}
psInheritable& psInheritable::operator=(psInheritable&& mov)
{
  PROFILE_FUNC();
  while(_children!=0) _children->SetParent(0);
  SetParent(mov._parent);
  _children=mov._children;
  mov._children=0;
  mov.SetParent(0);
  for(psInheritable* cur=_children; cur!=0; cur=cur->_lchild.next)
    cur->_parent=this;
  psRenderable::operator=(std::move(mov));
  psLocatable::operator=(std::move(mov));
  return *this;
}
void psInheritable::_sortchildren()
{
  uint32_t count = NumChildren();
  if(count > 0)
  {
    DYNARRAY(psInheritable*, buf, count);

    uint32_t i = 0;
    for(psInheritable* cur = _children; i < count && cur != 0; cur = cur->_lchild.next)
      buf[i++] = cur;

    std::sort<psInheritable**>(buf, buf + count, &INHERITABLECOMP);

    psInheritable* prev = 0;
    --count; // this is legal because we know count is at least 1
    for(i = 0; i < count; ++i)
    {
      buf[i]->_lchild.prev = prev;
      buf[i]->_lchild.next = buf[i + 1];
    }
    buf[count]->_lchild.prev = buf[count - 1];
    buf[count]->_lchild.next = 0;
  }
  _internalflags |= INTERNALFLAG_SORTED; // children are now sorted
}
psRenderable* BSS_FASTCALL psInheritable::_getparent() const { return _parent; }

char BSS_FASTCALL psInheritable::_sort(psRenderable* r) const
{
  const psRenderable* l = this;
  if(r->_getparent())
  {
    psInheritable* rt = static_cast<psInheritable*>(r); // If you have a parent, you MUST be a psInheritable.
    uint32_t ld = _depth;
    uint32_t rd = rt->_depth;
    const psInheritable* lt = this;

    while(ld > rd) lt = lt->_parent;
    while(ld < rd) rt = rt->_parent;

    while(lt->_parent != rt->_parent)
    {
      lt = lt->_parent;
      rt = rt->_parent;
    }

    l = lt;
    r = rt;
  }
  else // otherwise r has no parent, so go all the way up our parent chain until we hit our top level parent to compare with.
  {
    const psInheritable* lt = this;
    while(lt->_parent) lt = lt->_parent;
    l = lt;
  }

  char c = SGNCOMPARE(l->_zorder, r->_zorder);
  if(!c) c = SGNCOMPARE(l, r);
  return c;
}

uint32_t psInheritable::NumChildren() const
{
  uint32_t count = 0;
  for(psInheritable* cur = _children; cur != 0; cur = cur->_lchild.next)
    ++count;
  return count;
}
inline psTex* const* psInheritable::GetRenderTargets() const
{
  if(_rts.Capacity())
    return _rts;
  if(_parent != 0)
    return _parent->GetRenderTargets();
  return !_pass ? 0 : _pass->GetRenderTarget();
}
inline uint8_t psInheritable::NumRT() const
{
  if(_rts.Capacity())
    return _rts.Capacity();
  if(_parent != 0)
    return _parent->NumRT();
  return (_pass != 0 && _pass->GetRenderTarget()[0] != 0);
}
//psCamera* psInheritable::GetCamera() const { return !_parent?0:_parent->GetCamera(); }
