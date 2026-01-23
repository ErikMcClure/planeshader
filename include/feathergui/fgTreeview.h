// Copyright (c)2026 Erik McClure
// For conditions of distribution and use, see copyright notice in "feathergui.h"

#ifndef __FG_TREEVIEW_H__
#define __FG_TREEVIEW_H__

#include "fgScrollbar.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _FG_TREEITEM {
  fgControl control;
  fgElement arrow;
  size_t count;
  AbsVec spacing;
} fgTreeItem;

// A treeview visualizes a tree structure as a series of nested lists. 
typedef struct _FG_TREEVIEW {
  fgScrollbar scrollbar;
  AbsVec spacing;
#ifdef  __cplusplus
  inline operator fgElement*() { return &scrollbar.control.element; }
  inline fgElement* operator->() { return operator fgElement*(); }
#endif
} fgTreeview;

FG_EXTERN void fgTreeview_Init(fgTreeview* BUN_RESTRICT self, fgElement* BUN_RESTRICT parent, fgElement* BUN_RESTRICT next, const char* name, fgFlag flags, const fgTransform* transform, fgMsgType units);
FG_EXTERN void fgTreeview_Destroy(fgTreeview* self);
FG_EXTERN size_t fgTreeview_Message(fgTreeview* self, const FG_Msg* msg);

FG_EXTERN void fgTreeItem_Init(fgTreeItem* BUN_RESTRICT self, fgElement* BUN_RESTRICT parent, fgElement* BUN_RESTRICT next, const char* name, fgFlag flags, const fgTransform* transform, fgMsgType units);
FG_EXTERN size_t fgTreeItem_Message(fgTreeItem* self, const FG_Msg* msg);
FG_EXTERN void fgTreeItem_Destroy(fgTreeItem* self);

#ifdef  __cplusplus
}
#endif

#endif