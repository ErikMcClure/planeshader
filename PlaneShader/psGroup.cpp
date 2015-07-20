// Copyright ©2015 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in PlaneShader.h

#include "psGroup.h"

using namespace planeshader;

psGroup::psGroup(const DEF_GROUP& def) : psInheritable(def) 
{
  for(unsigned int i = 0; i < def.NumInheritables(); ++i)
    AddDef(*def.GetInheritable(i));
}

psGroup::psGroup(const psGroup& copy) : psInheritable(copy)
{ 
  copy.MapToChildren(bss_util::delegate<psInheritable*, psInheritable*>::From<psGroup, &psGroup::AddClone>(this));
}

psGroup::psGroup(psGroup&& mov) : psInheritable(std::move(mov)), _list(std::move(mov._list)) {}

psGroup::psGroup(const psVec3D& position, FNUM rotation, const psVec& pivot, FLAG_TYPE flags, int zorder, psStateblock* stateblock, psShader* shader, psPass* pass, psInheritable* parent) :
  psInheritable(position, rotation, pivot, flags, zorder, stateblock, shader, pass, parent)
{
}

psGroup::~psGroup()
{
  for(auto i = _list.begin(); i != _list.end(); ++i)
    delete _list.GetKey(*i);
}

psInheritable* BSS_FASTCALL psGroup::AddClone(psInheritable* inheritable)
{
  psInheritable* n = inheritable->Clone();
  AddRef(n);
  _list.Insert(n);
  return n;
}

psInheritable* BSS_FASTCALL psGroup::AddDef(const DEF_INHERITABLE& inheritable)
{
  psInheritable* n = inheritable.Spawn();
  AddRef(n);
  _list.Insert(n);
  return n;
}

void BSS_FASTCALL psGroup::AddRef(psInheritable* inheritable)
{
  inheritable->SetParent(this);
}

bool BSS_FASTCALL psGroup::Remove(psInheritable* inheritable)
{
  if(inheritable->GetParent() != this) return false;
  inheritable->SetParent(0);
  khiter_t iter = _list.Iterator(inheritable);
  if(_list.ExistsIter(iter)) {
    _list.RemoveIter(iter);
    delete inheritable;
  }
  return true;
}

void psGroup::_render() {}