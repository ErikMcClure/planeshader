// Copyright (c)2026 Erik McClure
// For conditions of distribution and use, see copyright notice in "feathergui.h"

#ifndef __FG_WINDOW_H__
#define __FG_WINDOW_H__

#include "fgButton.h"

#ifdef  __cplusplus
extern "C" {
#endif

enum FGWINDOW_FLAGS
{
  FGWINDOW_MINIMIZABLE = (FGCONTROL_DISABLE << 1),
  FGWINDOW_MAXIMIZABLE = (FGWINDOW_MINIMIZABLE << 1),
  FGWINDOW_RESIZABLE = (FGWINDOW_MAXIMIZABLE << 1),
  FGWINDOW_NOCAPTION = (FGWINDOW_RESIZABLE << 1),
  FGWINDOW_NOBORDER = (FGWINDOW_NOCAPTION << 1),
};

enum FGWINDOW_ACTIONS
{
  FGWINDOW_CLOSE = 0,
  FGWINDOW_MAXIMIZE,
  FGWINDOW_RESTORE,
  FGWINDOW_MINIMIZE,
  FGWINDOW_UNMINIMIZE,
};

struct _FG_BUTTON;

// A top-level window is an actual window with a titlebar that can be dragged and resized.
typedef struct _FG_WINDOW {
  fgControl control;
  fgText caption;
  fgButton btn_close;
  fgButton btn_restore;
  fgButton btn_minimize;
  CRect prevrect; // Stores where the window was before being maximized
  AbsVec offset; // offset from the mouse cursor for movement
  char dragged; // 1 if currently being dragged by mouse
  char maximized; // 1 if maximized
#ifdef  __cplusplus
  _FG_WINDOW(_FG_WINDOW&&) = default;
  _FG_WINDOW(const _FG_WINDOW&) = delete;
  _FG_WINDOW(fgElement* BUN_RESTRICT parent, fgElement* BUN_RESTRICT next, const char* name, fgFlag flags, std::optional<fgTransform> transform, fgMsgType units);
  ~_FG_WINDOW();

  _FG_WINDOW& operator=(_FG_WINDOW&&) = default;
  _FG_WINDOW& operator=(const _FG_WINDOW&) = delete;
  inline operator fgElement*() { return &control.element; }
  inline fgElement* operator->() { return operator fgElement*(); }
#endif
} fgWindow;

FG_EXTERN void fgWindow_Init(fgWindow* self, fgElement* BUN_RESTRICT parent, fgElement* BUN_RESTRICT next, const char* name, fgFlag flags, const fgTransform* transform, fgMsgType units);
FG_EXTERN void fgWindow_Destroy(fgWindow* self);
FG_EXTERN size_t fgWindow_Message(fgWindow* self, const FG_Msg* msg);

#ifdef  __cplusplus
}
#endif

#endif
