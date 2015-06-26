// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psInheritable.h"
#include "bss-util/profiler.h"

using namespace planeshader;

psInheritable::psInheritable(const psInheritable& copy) : psRenderable(copy), psLocatable(copy), _parent(0), _children(0)
{
  _lchild.next=_lchild.prev=0;
  SetParent(copy._parent);
}
psInheritable::psInheritable(psInheritable&& mov) : psRenderable(std::move(mov)), psLocatable(std::move(mov)), _parent(0), _children(mov._children)
{
  _lchild.next=_lchild.prev=0;
  SetParent(mov._parent);
  mov._children=0;
  mov.SetParent(0);
  for(psInheritable* cur=_children; cur!=0; cur=cur->_lchild.next)
    _children->_parent=this;
  SetPass(_pass); // psRenderable won't have been able to resolve this virtual function in its constructor
}
psInheritable::psInheritable(const DEF_INHERITABLE& def) : psRenderable(def), psLocatable(def), _parent(0), _children(0)
{
  _lchild.next=_lchild.prev=0;
  SetParent(def.parent);
}
psInheritable::psInheritable(const psVec3D& position, FNUM rotation, const psVec& pivot, FLAG_TYPE flags, int zorder, psStateblock* stateblock, psShader* shader, unsigned short pass, psInheritable* parent) : 
  psRenderable(flags,zorder,stateblock,shader,pass), psLocatable(position,rotation,pivot), _parent(0), _children(0)
{
  _lchild.next=_lchild.prev=0;
  SetParent(parent);
}
psInheritable::~psInheritable()
{
  while(_children!=0) _children->SetParent(0);
  SetParent(0);
}
void psInheritable::Render()
{
  for(psInheritable* cur=_children; cur!=0; cur=cur->_lchild.next)
    cur->Render();
}
void psInheritable::SetParent(psInheritable* parent)
{
  PROFILE_FUNC();
  if(parent==_parent) return;
  if(_parent)
    bss_util::AltLLRemove<psInheritable,&GetLLBase>(this, _parent->_children);

  _lchild.next=_lchild.prev=0;
  //if(_pinfo) _pinfo->p=0;
  //_pinfo=0; // Changing the parent forces an insertion
  _parent=parent;
  if(_parent==this) // No circular reference insanity allowed!
    _parent=0;

  if(_parent!=0)
  {
    SetPass(_parent->_pass);
    bss_util::AltLLAdd<psInheritable, &GetLLBase>(this, _parent->_children);
  }
}

void BSS_FASTCALL psInheritable::SetPass(unsigned short pass)
{
  PROFILE_FUNC();
  psRenderable::SetPass(pass);
  for(psInheritable* cur=_children; cur!=0; cur=cur->_lchild.next)
    cur->SetPass(pass);
}
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
    _children->_parent=this;
  psRenderable::operator=(std::move(mov));
  psLocatable::operator=(std::move(mov));
  return *this;
}