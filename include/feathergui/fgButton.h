// Copyright (c)2026 Erik McClure
// For conditions of distribution and use, see copyright notice in "feathergui.h"

#ifndef __FG_BUTTON_H__
#define __FG_BUTTON_H__

#include "fgControl.h"
#include "fgText.h"

#ifdef  __cplusplus
extern "C" {
#endif

enum FGBUTTON_FLAGS
{
  FGBUTTON_NOFOCUS = (FGCONTROL_DISABLE << 1),
};

// A button is usually implemented as a simple background design that takes a static and displays it in the center of the button.
typedef struct _FG_BUTTON {
  fgControl control;
  fgText text; // text displayed in button
#ifdef  __cplusplus
  _FG_BUTTON(_FG_BUTTON&&) = default;
  _FG_BUTTON(const _FG_BUTTON&) = delete;
  _FG_BUTTON(fgElement* BUN_RESTRICT parent, fgElement* BUN_RESTRICT next, const char* name, fgFlag flags, std::optional<fgTransform> transform, fgMsgType units);
  ~_FG_BUTTON();

  _FG_BUTTON& operator=(_FG_BUTTON&&) = default;
  _FG_BUTTON& operator=(const _FG_BUTTON&) = delete;

  inline operator fgElement*() { return &control.element; }
  inline fgElement* operator->() { return operator fgElement*(); }
#endif
} fgButton;

FG_EXTERN void fgButton_Init(fgButton* BUN_RESTRICT self, fgElement* BUN_RESTRICT parent, fgElement* BUN_RESTRICT next, const char* name, fgFlag flags, const fgTransform* transform, fgMsgType units);
FG_EXTERN void fgButton_Destroy(fgButton* self);
FG_EXTERN size_t fgButton_Message(fgButton* self, const FG_Msg* msg);

#ifdef  __cplusplus
}
#endif

#endif
