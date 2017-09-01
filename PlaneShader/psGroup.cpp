// Copyright ©2017 Black Sphere Studios
// For conditions of distribution and use, see copyright notice in ps_dec.h

#include "psGroup.h"
#include "bss-util/profiler.h"
#include "psLayer.h"
#include <algorithm>

using namespace planeshader;

psGroup::psGroup(const psGroup& copy) : psRenderable(copy), psLocatable(copy), _children(0)
{
  _children.Reserve(copy._children.Length());
  for(size_t i = 0; i < copy._children.Length(); ++i)
    Add(copy._children[i]->Clone());
}
psGroup::psGroup(psGroup&& mov) : psRenderable(std::move(mov)), psLocatable(std::move(mov)), _children(std::move(_children))
{}
psGroup::psGroup(const psVec3D& position, FNUM rotation, const psVec& pivot, psFlag flags, int zorder, psStateblock* stateblock, psShader* shader, psLayer* pass) :
  psRenderable(flags, zorder, stateblock, shader, pass), psLocatable(position, rotation, pivot), _children(0)
{}
psGroup::~psGroup()
{
  for(size_t i = 0; i < _children.Length(); ++i)
    delete _children[i];
}
psRenderable* psGroup::Add(const psRenderable* renderable)
{
  psRenderable* r = renderable->Clone();
  r->SetPass(GetLayer());
  _children.Insert(r);
  return r;
}
void psGroup::_render(const psTransform2D& parent)
{
  psTransform2D n = parent.Push(*this);

  for(size_t i = 0; i < _children.Length(); ++i)
    _children[i]->Render(&n);
}

psGroup& psGroup::operator=(const psGroup& copy)
{
  PROFILE_FUNC();
  for(size_t i = 0; i < _children.Length(); ++i)
    delete _children[i];
  psRenderable::operator=(copy);
  psLocatable::operator=(copy);
  _children.Reserve(copy._children.Length());
  for(size_t i = 0; i < copy._children.Length(); ++i)
    Add(copy._children[i]->Clone());
  return *this;
}
psGroup& psGroup::operator=(psGroup&& mov)
{
  PROFILE_FUNC();
  for(size_t i = 0; i < _children.Length(); ++i)
    delete _children[i];
  psRenderable::operator=(std::move(mov));
  psLocatable::operator=(std::move(mov));
  _children = std::move(mov._children);
  return *this;
}