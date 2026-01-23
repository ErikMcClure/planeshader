// Copyright (c)2026 Erik McClure
// For conditions of distribution and use, see copyright notice in "feathergui.h"

#include "feathercpp.h"
#include "feathergui/fgSkinTree.h"
#include "buntils/ArraySort.h"

using StyleArraySort = bun::ArraySort<fgSkinTree::fgStylePair>;

void fgSkinElement_Init(fgSkinElement* self, const char* type, fgFlag flags, const fgTransform* transform, fgMsgType units, int order)
{
  self->type = fgCopyText(type, __FILE__, __LINE__);
  self->transform = *transform;
  self->units = units;
  self->flags = flags;
  self->order = order;
  self->skin = 0;
  fgStyle_Init(&self->style);
}

void fgSkinElement_InitCopy(fgSkinElement* self, const fgSkinElement* from)
{
  self->transform = from->transform;
  self->units = from->units;
  self->flags = from->flags;
  self->type = fgCopyText(from->type, __FILE__, __LINE__);
  fgStyle_InitCopy(&self->style, &from->style);
  self->order = from->order;
  self->skin = from->skin;
}

void fgSkinElement_ResolveCopy(fgSkinElement* self, struct _FG_SKIN_BASE* parent)
{
  if(self->skin)
    self->skin = parent->GetAnySkin(self->skin->base.name);
}

void fgSkinElement_Destroy(fgSkinElement* self)
{
  if(self->type) fgFreeText(self->type, __FILE__, __LINE__);
  fgStyle_Destroy(&self->style);
}

void fgSkinTree_Init(fgSkinTree* self)
{
  bun::bun_Fill(*self, 0);
}
void fgSkinTree_InitCopy(fgSkinTree* self, const fgSkinTree* from)
{
  new(&self->children) fgSkinLayoutArray(reinterpret_cast<const fgSkinLayoutArray&>(from->children));
  self->stylemask = from->stylemask;
  self->styles.s = from->styles.s;
  self->styles.l = from->styles.l;
  self->styles.p = bun::bun_Malloc<fgSkinTree::fgStylePair>(self->styles.s); // do NOT use fgmalloc, this is not freed via fgfree
  for(size_t i = 0; i < from->styles.l; ++i)
  {
    self->styles.p[i].map = from->styles.p[i].map;
    fgStyle_InitCopy(&self->styles.p[i].style, &from->styles.p[i].style);
  }
}
void fgSkinTree_ResolveCopy(fgSkinTree* self, struct _FG_SKIN_BASE* parent)
{
  for(size_t i = 0; i < self->children.l; ++i)
  {
    fgSkinElement_ResolveCopy(&self->children.p[i].element, parent);
    fgSkinTree_ResolveCopy(&self->children.p[i].tree, parent);
  }
}

void fgSkinTree_Destroy(fgSkinTree* self)
{
  reinterpret_cast<fgSkinLayoutArray&>(self->children).~ArraySort();

  for(size_t i = 0; i < self->styles.l; ++i)
    fgStyle_Destroy(&self->styles.p[i].style);
  ((StyleArraySort&)self->styles).~ArraySort();
}

size_t fgSkinTree_AddChild(fgSkinTree* self, const char* type, fgFlag flags, const fgTransform* transform, fgMsgType units, int order)
{
  return ((fgSkinLayoutArray&)self->children).Insert(fgSkinLayout(type, flags, transform, units, order));
}
char fgSkinTree_RemoveChild(fgSkinTree* self, FG_UINT child)
{
  return DynArrayRemove((fgSkinLayoutArray&)self->children, child);
}
fgSkinLayout* fgSkinTree_GetChild(const fgSkinTree* self, FG_UINT child)
{
  return self->children.p + child;
}

FG_UINT fgSkinTree_AddStyle(fgSkinTree* self, const char* names)
{
  FG_UINT style = fgStyle_GetAllNames(names);
  self->stylemask |= style;
  fgSkinTree::fgStylePair pair = { style, 0 };
  if(((StyleArraySort&)self->styles).Find(pair) == (size_t)~0)
    ((StyleArraySort&)self->styles).Insert(pair);
  return style;
}

char fgSkinTree_RemoveStyle(fgSkinTree* self, FG_UINT style)
{
  fgSkinTree::fgStylePair pair = { style, 0 };
  size_t i = ((StyleArraySort&)self->styles).Find(pair);
  if(i == (size_t)~0)
  {
    fgLog(FGLOG_INFO, "Tried to remove nonexistant style %u", style);
    return 0;
  }
  fgStyle_Destroy(&self->styles.p[i].style);
  return ((StyleArraySort&)self->styles).Remove(i);
}
fgStyle* fgSkinTree_GetStyle(const fgSkinTree* self, FG_UINT style)
{
  fgSkinTree::fgStylePair pair = { style, 0 };
  size_t i = ((StyleArraySort&)self->styles).Find(pair);
  return (i == (size_t)~0) ? 0 : (&self->styles.p[i].style);
}

fgSkinLayout::_FG_SKIN_LAYOUT(const char* type, fgFlag flags, const fgTransform* transform, fgMsgType units, int order) {
  bun::bun_Fill(*this, 0);
  fgSkinElement_Init(&element, type, flags, transform, units, order);
  instance = fgroot_instance->backend.fgCreate(type, 0, 0, 0, flags, (units == (fgMsgType)~0) ? 0 : transform, units);
}
fgSkinLayout::_FG_SKIN_LAYOUT(const _FG_SKIN_LAYOUT& from) {
  fgSkinElement_InitCopy(&element, &from.element);
  fgSkinTree_InitCopy(&tree, &from.tree);
  instance = fgroot_instance->backend.fgCreate(element.type, 0, 0, 0, element.flags, (element.units == (fgMsgType)~0) ? nullptr : &element.transform, element.units);
  _sendsubmsg<FG_SETSTYLE, void*, size_t>(instance, FGSETSTYLE_POINTER, (void*)&element.style, ~0);
}
fgSkinLayout::~_FG_SKIN_LAYOUT() {
  fgSkinElement_Destroy(&element);
  if (instance) VirtualFreeChild(instance);
  fgSkinTree_Destroy(&tree);
}

fgSkinLayout& fgSkinLayout::operator=(const fgSkinLayout& r) {
  this->~_FG_SKIN_LAYOUT();
  new(this) fgSkinLayout(r);
  return *this;
}


void fgSkinLayout_Init(fgSkinLayout* self, const char* type, fgFlag flags, const fgTransform* transform, fgMsgType units, int order)
{
  new(self) fgSkinLayout(type, flags, transform, units, order);
}

void fgSkinLayout_InitCopy(fgSkinLayout* self, const fgSkinLayout* from)
{
  new(self) fgSkinLayout(*from);
}

void fgSkinLayout_Destroy(fgSkinLayout* self)
{
  self->~_FG_SKIN_LAYOUT();
}
